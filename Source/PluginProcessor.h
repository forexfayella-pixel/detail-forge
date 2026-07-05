#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>
#include <vector>

// Faust-generated types (defined in the .cpp translation unit only).
class ClipEngine;
class MapUI;

//==============================================================================
// Detail Forge.
// Full parameter surface is declared here so the UI binds to real, automatable
// params. DSP wired so far: Saturator (voicing/drive/bias/mix, oversampled),
// clip Drive (Faust ADAA2), Input/Output gain, Bypass.
// The remaining params (limiter) are live parameters whose DSP stages land in M2-M3.
class DetailForgeProcessor : public juce::AudioProcessor
{
public:
    DetailForgeProcessor();
    ~DetailForgeProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Detail Forge"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Live meter levels (dBFS), read by the editor. -100 == silence.
    std::atomic<float> inLevelDb  { -100.0f };
    std::atomic<float> outLevelDb { -100.0f };
    std::atomic<float> grDb       { 0.0f };

    // --- Scope capture: lock-free ring of recent input/output (channel 0) ---
    static constexpr int kScopeCap = 8192;            // power of two
    static constexpr int kScopeMask = kScopeCap - 1;
    float inRing[kScopeCap]  {};
    float outRing[kScopeCap] {};
    std::atomic<uint32_t> scopeWrite { 0 };
    double currentSampleRate { 48000.0 };
    // Copy the most recent n samples of in/out into caller buffers (message thread).
    void readScope (float* inDst, float* outDst, int n) const noexcept;

    // ADAA2 hard clip introduces exactly 1 sample of group delay (measured).
    static constexpr int kLatencySamples = 1;

    // Probe the ACTUAL saturator+clip transfer for a ramp x in [-1,1], filling ys with N
    // output values. Message-thread only (uses a dedicated offline engine, never the audio
    // engines). This is the true DSP curve — including the morphing knee — not an approximation.
    void computeTransferCurve (std::vector<float>& ys, int N = 512);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::atomic<float>* satVoicing  = nullptr;    // "sat_voicing" (choice 0..2: Tube/Tape/Xfmr)
    std::atomic<float>* satDrive    = nullptr;    // "sat_drive" (dB)
    std::atomic<float>* satBias     = nullptr;    // "sat_bias" (%)
    std::atomic<float>* satMix      = nullptr;    // "sat_mix" (%)

    std::atomic<float>* clipDrive   = nullptr;   // "clip_drive" (dB) -> Faust
    std::atomic<float>* clipKnee    = nullptr;    // "clip_knee" (%)
    std::atomic<float>* clipCeiling = nullptr;    // "clip_ceiling" (dB)
    std::atomic<float>* osParam     = nullptr;    // "oversampling" (choice 0..4 -> 1x..16x)
    std::atomic<float>* adaaParam   = nullptr;    // "adaa_order" (choice 0..2)
    std::atomic<float>* inGain      = nullptr;    // "in_gain" (dB)
    std::atomic<float>* outGain     = nullptr;    // "out_gain" (dB)
    std::atomic<float>* bypass      = nullptr;    // "bypass" (0/1)

    std::vector<std::unique_ptr<ClipEngine>> engines;    // one per channel
    std::vector<std::unique_ptr<MapUI>>      maps;
    std::vector<float*> driveZones, kneeZones, ceilingZones, orderZones;  // cached Faust zones

    // Dedicated offline engine + resolved zones for the transfer-curve probe (message thread).
    std::unique_ptr<ClipEngine> probeEngine;
    std::unique_ptr<MapUI>      probeMap;
    float *pDrive = nullptr, *pKnee = nullptr, *pCeil = nullptr, *pOrder = nullptr;

    // Oversampling: index 1..4 -> orders 1..4 (2x/4x/8x/16x); index 0 -> 1x (bypass).
    std::vector<std::unique_ptr<juce::dsp::Oversampling<float>>> oversamplers;
    int lastReportedLatency = -1;

    // Dedicated 4x oversamplers for TRUE-PEAK (dBTP) metering of channel 0, in and out.
    // Independent of the audio-path oversampling so metering never couples to it. A meter
    // must see every sample, so this runs on the audio thread (peak-only, no downsample).
    std::unique_ptr<juce::dsp::Oversampling<float>> tpUpIn, tpUpOut;
    juce::AudioBuffer<float> tpScratchIn, tpScratchOut;

    // Per-channel one-pole DC blocker state (output hygiene).
    std::vector<float> dcX1, dcY1;

    // Pre-tap delay line (channel 0): delays the captured dry input by the plugin's
    // current internal latency L so the scope's dry (inRing) and wet (outRing) taps are
    // sample-aligned. Sized to the max possible L; L is applied as a movable read tap, so
    // switching oversampling factor never reallocates. See readScope / processBlock.
    std::vector<float> preTapDelay;
    int preTapMask = 0;   // (pow2 size) - 1
    int preTapPos  = 0;   // write index

    void runClip (float* data, int numSamples, int ch, float driveDb, float kneeN, float ceilDb, float orderN) noexcept;
    void runSat  (float* data, int numSamples, int voicing, float g, float bias, float mix) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DetailForgeProcessor)
};
