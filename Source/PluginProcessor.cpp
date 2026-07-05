#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

// Faust engine (generated from Source/dsp/clip.dsp by CMake).
#include "faust/gui/meta.h"
#include "faust/gui/MapUI.h"
#include "faust/dsp/dsp.h"
#include "ClipEngine.h"

using APF  = juce::AudioParameterFloat;
using APC  = juce::AudioParameterChoice;
using APB  = juce::AudioParameterBool;
using Range = juce::NormalisableRange<float>;

DetailForgeProcessor::DetailForgeProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    satVoicing  = apvts.getRawParameterValue ("sat_voicing");
    satDrive    = apvts.getRawParameterValue ("sat_drive");
    satBias     = apvts.getRawParameterValue ("sat_bias");
    satMix      = apvts.getRawParameterValue ("sat_mix");
    clipDrive   = apvts.getRawParameterValue ("clip_drive");
    clipKnee    = apvts.getRawParameterValue ("clip_knee");
    clipCeiling = apvts.getRawParameterValue ("clip_ceiling");
    osParam     = apvts.getRawParameterValue ("oversampling");
    adaaParam   = apvts.getRawParameterValue ("adaa_order");
    inGain      = apvts.getRawParameterValue ("in_gain");
    outGain     = apvts.getRawParameterValue ("out_gain");
    bypass      = apvts.getRawParameterValue ("bypass");
}

DetailForgeProcessor::~DetailForgeProcessor() = default;

// True-peak (dBTP) of a mono block: 4x-oversample (BS.1770-style) and take the max abs of the
// reconstructed signal. Float path, so no −12 dB headroom pad is needed. Peak-only: we discard
// the upsampled block (no downsample). Not wait-free-allocating — scratch is pre-sized.
static float truePeakDb (juce::dsp::Oversampling<float>& os, juce::AudioBuffer<float>& scratch,
                         const float* src, int n) noexcept
{
    if (n <= 0) return -100.0f;
    scratch.copyFrom (0, 0, src, n);
    float* chans[1] = { scratch.getWritePointer (0) };
    juce::dsp::AudioBlock<float> blk (chans, 1, (size_t) n);
    auto up = os.processSamplesUp (blk);
    float pk = 0.0f;
    const int m = (int) up.getNumSamples();
    for (int i = 0; i < m; ++i) pk = juce::jmax (pk, std::abs (up.getSample (0, i)));
    return juce::Decibels::gainToDecibels (pk, -100.0f);
}

juce::AudioProcessorValueTreeState::ParameterLayout DetailForgeProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    auto dB = [] (const char* l) { return juce::AudioParameterFloatAttributes{}.withLabel (l); };

    // Global
    p.add (std::make_unique<APF>(juce::ParameterID{"in_gain",1},  "Input",  Range{-12.f,12.f,0.01f}, 0.f, dB("dB")));
    p.add (std::make_unique<APF>(juce::ParameterID{"out_gain",1}, "Output", Range{-12.f,12.f,0.01f}, 0.f, dB("dB")));
    p.add (std::make_unique<APB>(juce::ParameterID{"bypass",1},   "Bypass", false));

    // Saturator
    p.add (std::make_unique<APC>(juce::ParameterID{"sat_voicing",1}, "Voicing",
                                 juce::StringArray{"Tube","Tape","Xfmr"}, 0));
    p.add (std::make_unique<APF>(juce::ParameterID{"sat_drive",1}, "Sat Drive", Range{0.f,24.f,0.01f}, 6.f,  dB("dB")));
    p.add (std::make_unique<APF>(juce::ParameterID{"sat_bias",1},  "Sat Bias",  Range{0.f,100.f,0.1f}, 20.f, dB("%")));
    p.add (std::make_unique<APF>(juce::ParameterID{"sat_mix",1},   "Sat Mix",   Range{0.f,100.f,0.1f}, 100.f,dB("%")));

    // Clipper
    p.add (std::make_unique<APF>(juce::ParameterID{"clip_drive",1},   "Clip Drive",   Range{0.f,36.f,0.01f},  2.f, dB("dB")));
    p.add (std::make_unique<APF>(juce::ParameterID{"clip_knee",1},    "Clip Knee",    Range{0.f,100.f,0.1f}, 35.f, dB("%")));
    p.add (std::make_unique<APF>(juce::ParameterID{"clip_ceiling",1}, "Clip Ceiling", Range{-18.f,0.f,0.01f},-3.f, dB("dB")));
    p.add (std::make_unique<APC>(juce::ParameterID{"oversampling",1}, "Oversampling",
                                 juce::StringArray{"1x","2x","4x","8x","16x"}, 2));
    p.add (std::make_unique<APC>(juce::ParameterID{"adaa_order",1},   "ADAA Order",
                                 juce::StringArray{"Off","1st","2nd"}, 2));

    // Limiter
    p.add (std::make_unique<APF>(juce::ParameterID{"lim_threshold",1}, "Lim Threshold", Range{-24.f,0.f,0.01f}, -6.f, dB("dB")));
    p.add (std::make_unique<APF>(juce::ParameterID{"lim_ceiling",1},   "Lim Ceiling",   Range{-12.f,0.f,0.01f}, -1.f, dB("dB")));
    p.add (std::make_unique<APF>(juce::ParameterID{"lim_release",1},   "Lim Release",   Range{1.f,500.f,0.1f},  60.f, dB("ms")));

    return p;
}

void DetailForgeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    static_assert (sizeof (FAUSTFLOAT) == sizeof (float),
                   "M0 engine is single precision; driveZones stores float*.");

    currentSampleRate = sampleRate;
    engines.clear(); maps.clear();
    driveZones.clear(); kneeZones.clear(); ceilingZones.clear(); orderZones.clear();
    const int nch = juce::jmax (1, getTotalNumOutputChannels());
    for (int c = 0; c < nch; ++c)
    {
        auto e = std::make_unique<ClipEngine>();
        e->init ((int) sampleRate);
        auto m = std::make_unique<MapUI>();
        e->buildUserInterface (m.get());
        // Resolve zones once here (string lookup off the audio thread).
        driveZones.push_back   (m->getParamZone ("/DetailForgeClip/Drive"));
        kneeZones.push_back    (m->getParamZone ("/DetailForgeClip/Knee"));
        ceilingZones.push_back (m->getParamZone ("/DetailForgeClip/Ceiling"));
        orderZones.push_back   (m->getParamZone ("/DetailForgeClip/Order"));
        engines.push_back (std::move (e));
        maps.push_back (std::move (m));
    }

    // Offline probe engine (message-thread transfer-curve probing; separate from the audio path).
    probeEngine = std::make_unique<ClipEngine>();
    probeEngine->init ((int) sampleRate);
    probeMap = std::make_unique<MapUI>();
    probeEngine->buildUserInterface (probeMap.get());
    pDrive = probeMap->getParamZone ("/DetailForgeClip/Drive");
    pKnee  = probeMap->getParamZone ("/DetailForgeClip/Knee");
    pCeil  = probeMap->getParamZone ("/DetailForgeClip/Ceiling");
    pOrder = probeMap->getParamZone ("/DetailForgeClip/Order");

    // Oversamplers for orders 1..4 (2x/4x/8x/16x). Low-latency polyphase IIR,
    // integer latency so host reporting is exact.
    oversamplers.clear();
    for (int order = 1; order <= 4; ++order)
    {
        auto os = std::make_unique<juce::dsp::Oversampling<float>> (
            (size_t) nch, (size_t) order,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
        os->initProcessing ((size_t) juce::jmax (1, samplesPerBlock));
        os->reset();
        oversamplers.push_back (std::move (os));
    }

    dcX1.assign ((size_t) nch, 0.0f);
    dcY1.assign ((size_t) nch, 0.0f);

    // Size the pre-tap delay line to the largest internal latency across all OS factors
    // (integer latency is guaranteed — oversamplers use useIntegerLatency=true), rounded
    // up to a power of two so the read tap can wrap with a mask.
    int maxLat = kLatencySamples;
    for (auto& os : oversamplers)
        maxLat = juce::jmax (maxLat, (int) std::lround (os->getLatencyInSamples()));
    int cap = 1;
    while (cap <= maxLat + 1) cap <<= 1;
    preTapDelay.assign ((size_t) cap, 0.0f);
    preTapMask = cap - 1;
    preTapPos  = 0;

    // True-peak metering: dedicated mono 4x (=2 stages) oversamplers, one per meter tap.
    using OS = juce::dsp::Oversampling<float>;
    tpUpIn  = std::make_unique<OS> ((size_t) 1, (size_t) 2, OS::filterHalfBandPolyphaseIIR, true, false);
    tpUpOut = std::make_unique<OS> ((size_t) 1, (size_t) 2, OS::filterHalfBandPolyphaseIIR, true, false);
    const int mb = juce::jmax (1, samplesPerBlock);
    tpUpIn->initProcessing  ((size_t) mb); tpUpIn->reset();
    tpUpOut->initProcessing ((size_t) mb); tpUpOut->reset();
    tpScratchIn.setSize  (1, mb);
    tpScratchOut.setSize (1, mb);

    // Report the correct initial latency for the current oversampling setting.
    const int osIdx = osParam != nullptr ? (int) osParam->load() : 0;
    lastReportedLatency = (osIdx >= 1 && osIdx <= (int) oversamplers.size())
        ? (int) std::lround (oversamplers[(size_t) (osIdx - 1)]->getLatencyInSamples())
        : kLatencySamples;
    setLatencySamples (lastReportedLatency);
}

void DetailForgeProcessor::runClip (float* data, int numSamples, int ch,
                                    float driveDb, float kneeN, float ceilDb, float orderN) noexcept
{
    if (auto* z = driveZones[(size_t) ch])   *z = (FAUSTFLOAT) driveDb;
    if (auto* z = kneeZones[(size_t) ch])    *z = (FAUSTFLOAT) kneeN;
    if (auto* z = ceilingZones[(size_t) ch]) *z = (FAUSTFLOAT) ceilDb;
    if (auto* z = orderZones[(size_t) ch])   *z = (FAUSTFLOAT) orderN;
    engines[(size_t) ch]->compute (numSamples, &data, &data);   // in-place is safe
}

// Stage 1 saturator (per-sample waveshaper). Character = transfer-shape + symmetry.
//   Tube  — tanh, asymmetric via bias  (even + odd harmonics)
//   Tape  — rational u/(1+|u|), gentle knee (softer compression)
//   Xfmr  — (2/pi)atan, harder shoulders, asymmetric via bias
// bias shifts the input into the curve → even harmonics; the -f(bias) term
// removes the DC it introduces (the output DC blocker is a second safety net).
// Divide by f(g) so peaks stay ~bounded to unity before the clipper.
// Runs inside the oversampled region, so oversampling anti-aliases it.
void DetailForgeProcessor::runSat (float* data, int numSamples, int voicing,
                                   float g, float bias, float mix) noexcept
{
    // Nothing to do at unity drive with no bias and no wet signal.
    auto f = [voicing] (float u) noexcept -> float
    {
        switch (voicing)
        {
            case 1:  return u / (1.0f + std::abs (u));                                     // Tape
            case 2:  return (2.0f / juce::MathConstants<float>::pi) * std::atan (u);       // Xfmr
            default: return std::tanh (u);                                                 // Tube
        }
    };

    const float fBias = f (bias);
    const float norm  = 1.0f / juce::jmax (1.0e-4f, f (g));   // peak-normalise
    const float dry   = 1.0f - mix;

    for (int i = 0; i < numSamples; ++i)
    {
        const float x   = data[i];
        const float wet = (f (g * x + bias) - fBias) * norm;
        data[i] = dry * x + mix * wet;
    }
}

void DetailForgeProcessor::releaseResources()
{
    engines.clear(); maps.clear();
    driveZones.clear(); kneeZones.clear(); ceilingZones.clear(); orderZones.clear();
    oversamplers.clear();
    tpUpIn.reset(); tpUpOut.reset();
}

bool DetailForgeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& main = layouts.getMainOutputChannelSet();
    if (main != juce::AudioChannelSet::mono() && main != juce::AudioChannelSet::stereo())
        return false;
    return main == layouts.getMainInputChannelSet();
}

void DetailForgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), (int) engines.size());

    // How many samples the wet output lags the dry input this block. True bypass adds no
    // delay; otherwise it is this block's reported latency (OS group delay, or the base
    // ADAA latency at 1x). We delay the captured dry by exactly this so the scope aligns.
    const bool bypassedNow = (bypass != nullptr && *bypass > 0.5f);
    const int  osIdxNow    = osParam != nullptr ? (int) osParam->load() : 0;
    int captureLatency = 0;
    if (! bypassedNow)
        captureLatency = (osIdxNow >= 1 && osIdxNow <= (int) oversamplers.size())
            ? (int) std::lround (oversamplers[(size_t) (osIdxNow - 1)]->getLatencyInSamples())
            : kLatencySamples;
    captureLatency = juce::jlimit (0, preTapMask, captureLatency);

    // Capture the pre-processing input (channel 0), delayed by captureLatency, into the ring.
    const uint32_t w = scopeWrite.load (std::memory_order_relaxed);
    if (numChannels > 0)
    {
        const float* in0 = buffer.getReadPointer (0);
        for (int i = 0; i < numSamples; ++i)
        {
            preTapDelay[(size_t) preTapPos] = in0[i];                 // push newest
            const int rd = (preTapPos - captureLatency) & preTapMask; // read L samples back
            inRing[(w + (uint32_t) i) & kScopeMask] = preTapDelay[(size_t) rd];
            preTapPos = (preTapPos + 1) & preTapMask;
        }
    }

    // Input true-peak (dBTP, pre-gain) for the IN meter.
    const float inTPdb = (numChannels > 0)
        ? truePeakDb (*tpUpIn, tpScratchIn, buffer.getReadPointer (0), numSamples) : -100.0f;

    if (bypass != nullptr && *bypass > 0.5f)
    {
        if (numChannels > 0)
        {
            const float* in0 = buffer.getReadPointer (0);
            for (int i = 0; i < numSamples; ++i)
                outRing[(w + (uint32_t) i) & kScopeMask] = in0[i];
        }
        scopeWrite.store (w + (uint32_t) numSamples, std::memory_order_release);
        // Output == input in true bypass; still run tpUpOut (on the input) to keep its state warm.
        const float outTPdb = (numChannels > 0)
            ? truePeakDb (*tpUpOut, tpScratchOut, buffer.getReadPointer (0), numSamples) : inTPdb;
        inLevelDb.store  (inTPdb);
        outLevelDb.store (outTPdb);
        grDb.store (0.0f);
        return; // true bypass
    }

    const float ig = juce::Decibels::decibelsToGain (inGain  != nullptr ? inGain->load()  : 0.0f);
    const float og = juce::Decibels::decibelsToGain (outGain != nullptr ? outGain->load() : 0.0f);
    const float driveDb = clipDrive   != nullptr ? clipDrive->load()          : 0.0f;
    const float kneeN   = clipKnee    != nullptr ? clipKnee->load() * 0.01f    : 0.35f; // % -> 0..1
    const float ceilDb  = clipCeiling != nullptr ? clipCeiling->load()         : -3.0f;
    const float orderN  = adaaParam   != nullptr ? adaaParam->load()           : 2.0f;  // 0/1/2
    const int   osIndex = osParam     != nullptr ? (int) osParam->load()       : 0;     // 0..4

    // Saturator (stage 1). Drive dB -> linear; bias %/ceil into 0..0.7; mix %/->0..1.
    const int   satVoice = satVoicing != nullptr ? (int) satVoicing->load()     : 0;     // 0/1/2
    const float satG     = juce::Decibels::decibelsToGain (satDrive != nullptr ? satDrive->load() : 0.0f);
    const float satB     = (satBias != nullptr ? satBias->load() : 0.0f) * 0.01f * 0.7f;
    const float satMixN  = (satMix  != nullptr ? satMix->load()  : 100.0f) * 0.01f;
    const bool  satActive = satMixN > 0.0005f && (satG > 1.0001f || satB > 1.0e-4f);

    if (ig != 1.0f)
        buffer.applyGain (ig);

    int reportedLatency = kLatencySamples;
    if (osIndex <= 0 || osIndex > (int) oversamplers.size())
    {
        // 1x — run sat then clip at the base rate.
        for (int c = 0; c < numChannels; ++c)
        {
            auto* d = buffer.getWritePointer (c);
            if (satActive) runSat (d, numSamples, satVoice, satG, satB, satMixN);
            runClip (d, numSamples, c, driveDb, kneeN, ceilDb, orderN);
        }
    }
    else
    {
        // Nx — upsample, run sat then clip at the higher rate (best anti-aliasing), downsample.
        auto& os = *oversamplers[(size_t) (osIndex - 1)];
        juce::dsp::AudioBlock<float> block (buffer);
        auto sub = block.getSubsetChannelBlock (0, (size_t) numChannels);
        auto up  = os.processSamplesUp (sub);
        const int osSamples = (int) up.getNumSamples();
        for (int c = 0; c < numChannels; ++c)
        {
            auto* d = up.getChannelPointer ((size_t) c);
            if (satActive) runSat (d, osSamples, satVoice, satG, satB, satMixN);
            runClip (d, osSamples, c, driveDb, kneeN, ceilDb, orderN);
        }
        os.processSamplesDown (sub);
        reportedLatency = (int) std::lround (os.getLatencyInSamples());
    }

    // DC blocker (one-pole highpass, ~a few Hz) — output hygiene.
    for (int c = 0; c < numChannels; ++c)
    {
        float* d = buffer.getWritePointer (c);
        float x1 = dcX1[(size_t) c], y1 = dcY1[(size_t) c];
        for (int i = 0; i < numSamples; ++i)
        {
            const float x = d[i];
            const float y = x - x1 + 0.9995f * y1;
            x1 = x; y1 = y; d[i] = y;
        }
        dcX1[(size_t) c] = x1; dcY1[(size_t) c] = y1;
    }

    if (og != 1.0f)
        buffer.applyGain (og);

    if (reportedLatency != lastReportedLatency)
    {
        setLatencySamples (reportedLatency);   // rare (only when OS factor changes)
        lastReportedLatency = reportedLatency;
    }

    // Capture the processed output (channel 0) into the scope ring; advance once.
    if (numChannels > 0)
    {
        const float* out0 = buffer.getReadPointer (0);
        for (int i = 0; i < numSamples; ++i)
            outRing[(w + (uint32_t) i) & kScopeMask] = out0[i];
    }
    scopeWrite.store (w + (uint32_t) numSamples, std::memory_order_release);

    const float outTPdb = (numChannels > 0)
        ? truePeakDb (*tpUpOut, tpScratchOut, buffer.getReadPointer (0), numSamples) : -100.0f;
    inLevelDb.store  (inTPdb);
    outLevelDb.store (outTPdb);
    grDb.store (0.0f); // limiter not built yet
}

void DetailForgeProcessor::readScope (float* inDst, float* outDst, int n) const noexcept
{
    const uint32_t w = scopeWrite.load (std::memory_order_acquire);
    for (int k = 0; k < n; ++k)
    {
        const uint32_t idx = (w - (uint32_t) n + (uint32_t) k) & kScopeMask;
        inDst[k]  = inRing[idx];
        outDst[k] = outRing[idx];
    }
}

void DetailForgeProcessor::computeTransferCurve (std::vector<float>& ys, int N)
{
    N = juce::jmax (2, N);
    ys.resize ((size_t) N);
    if (probeEngine == nullptr) { std::fill (ys.begin(), ys.end(), 0.0f); return; }

    // Snapshot the live saturator + clip params.
    const int   voice  = satVoicing != nullptr ? (int) satVoicing->load() : 0;
    const float sG     = juce::Decibels::decibelsToGain (satDrive != nullptr ? satDrive->load() : 0.0f);
    const float sB     = (satBias != nullptr ? satBias->load() : 0.0f) * 0.01f * 0.7f;
    const float sMix   = (satMix  != nullptr ? satMix->load()  : 100.0f) * 0.01f;
    const bool  satOn  = sMix > 0.0005f && (sG > 1.0001f || sB > 1.0e-4f);
    const float driveDb = clipDrive   != nullptr ? clipDrive->load()       : 0.0f;
    const float kneeN   = clipKnee    != nullptr ? clipKnee->load() * 0.01f : 0.35f;
    const float ceilDb  = clipCeiling != nullptr ? clipCeiling->load()      : -3.0f;
    const float orderN  = adaaParam   != nullptr ? adaaParam->load()        : 2.0f;

    // Build a monotonic ramp x in [-1,1] (slowly varying → ADAA output ≈ instantaneous curve),
    // push it through the SAME saturator math and the SAME Faust clip engine the audio uses.
    // Prepend a short warm-up held at x=-1 so ADAA history is settled before the ramp starts —
    // otherwise the first sample after instanceClear() shows a startup transient at the left edge.
    constexpr int W = 16;
    std::vector<float> buf ((size_t) (W + N));
    for (int i = 0; i < W; ++i) buf[(size_t) i] = -1.0f;
    for (int i = 0; i < N; ++i) buf[(size_t) (W + i)] = -1.0f + 2.0f * (float) i / (float) (N - 1);

    if (satOn)
        runSat (buf.data(), W + N, voice, sG, sB, sMix);

    if (pDrive) *pDrive = driveDb;
    if (pKnee)  *pKnee  = kneeN;
    if (pCeil)  *pCeil  = ceilDb;
    if (pOrder) *pOrder = orderN;

    probeEngine->instanceClear();                 // zero ADAA history between probes
    float* d = buf.data();
    probeEngine->compute (W + N, &d, &d);

    for (int i = 0; i < N; ++i) ys[(size_t) i] = buf[(size_t) (W + i)];  // drop the warm-up
}

juce::AudioProcessorEditor* DetailForgeProcessor::createEditor()
{
    return new DetailForgeEditor (*this);
}

void DetailForgeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void DetailForgeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DetailForgeProcessor();
}
