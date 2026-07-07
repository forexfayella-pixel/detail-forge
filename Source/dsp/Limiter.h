#pragma once
// Detail Forge — stage 3: hand-written true-peak lookahead brickwall limiter.
//
// Design (see specs/2026-07-06-m2-true-peak-limiter.md):
//   - Runs at BASE rate, AFTER the clip oversampling region, stereo-LINKED gain reduction.
//   - Internal 4x polyphase true-peak detector (inter-sample peak) drives the gain computer.
//   - GUARANTEED ceiling: a sliding-MINIMUM of the required gain over the lookahead window is
//     applied to audio delayed by that window, so the gain is already down when the peak lands
//     (no overshoot). One-pole RELEASE for recovery; attack is the lookahead ramp itself.
//   - Threshold acts as drive: peaks are pulled to threshold, then an auto-makeup of
//     (ceiling/threshold) brings the result to the ceiling — lower threshold = louder.
//
// RT-safe: prepare() allocates for the MAX lookahead; setParams() only changes the active window
// length (no allocation), so automating lookahead never touches the heap on the audio thread.
#include <vector>
#include <cmath>
#include <algorithm>

class TruePeakLimiter
{
public:
    static constexpr float kMaxLookaheadMs = 12.0f;

    void prepare (double sampleRate, int numChannels)
    {
        sr  = sampleRate;
        nch = std::max (1, numChannels);
        buildDetector();
        detDelay = (protoLen - 1) / 2 / OS;                 // detector group delay (input samples)

        maxL = (int) std::lround (kMaxLookaheadMs / 1000.0 * sr) + detDelay + 1;
        ring = maxL + 1;
        delay.assign ((size_t) nch, std::vector<float> ((size_t) ring, 0.0f));
        detBuf.assign ((size_t) nch, std::vector<float> ((size_t) protoLen, 0.0f));
        dqV.assign ((size_t) (maxL + 2), 1.0f);
        dqI.assign ((size_t) (maxL + 2), 0);

        setParams (thrDb_, ceilDb_, relMs_, lookMs_, /*force*/ true);
        reset();
    }

    // threshold/ceiling in dB, release in ms, lookahead in ms (0..12).
    void setParams (float thrDb, float ceilDb, float relMs, float lookMs, bool force = false)
    {
        thrDb_ = thrDb; ceilDb_ = ceilDb; relMs_ = relMs; lookMs_ = lookMs;
        const float margin = db2lin (-0.2f);                // safety for residual inter-sample detector error
        thrTgt  = db2lin (thrDb);                           // targets; thrLin/ceilLin ramp toward these
        ceilTgt = db2lin (ceilDb) * margin;
        relCoef = 1.0f - std::exp (-1.0f / std::max (1.0f, (relMs / 1000.0f) * (float) sr));
        smCoef  = 1.0f - std::exp (-1.0f / std::max (1.0f, 0.005f * (float) sr));   // 5 ms param smoothing
        if (force) { thrLin = thrTgt; ceilLin = ceilTgt; }
        int look = (int) std::lround ((double) juceClamp (lookMs, 0.0f, kMaxLookaheadMs) / 1000.0 * sr);
        int newL = std::min (maxL, look + detDelay + 1);
        // Window change: just update L. pushMin's own front-eviction adapts to the new window on the
        // next sample — do NOT zero the deque (that would momentarily under-populate the min-window
        // and let a peak slip past the ceiling). prepare()'s reset() handles the clean-start case.
        if (force || newL != L) L = std::max (1, newL);
    }

    void reset()
    {
        for (auto& d : delay)  std::fill (d.begin(), d.end(), 0.0f);
        for (auto& d : detBuf) std::fill (d.begin(), d.end(), 0.0f);
        detPos = 0; wpos = 0; nowIdx = 0; dqHead = 0; dqSize = 0;
        gState = 1.0f; grDb = 0.0f;
        thrLin = thrTgt; ceilLin = ceilTgt;   // start settled
    }

    int   getLatencySamples()    const noexcept { return L; }
    int   getMaxLatencySamples() const noexcept { return maxL; }   // for host buffer sizing
    float getGainReductionDb()   const noexcept { return grDb; }

    // In-place, stereo-linked. chans: numCh pointers of n samples each.
    void process (float* const* chans, int numCh, int n) noexcept
    {
        numCh = std::min (numCh, nch);
        float minGain = 1.0f;
        for (int i = 0; i < n; ++i)
        {
            // 1. push current sample into each detector ring
            for (int c = 0; c < numCh; ++c) detBuf[(size_t) c][(size_t) detPos] = chans[c][i];

            // 2. true-peak (linked): max |4x-interpolated value| across phases and channels
            float tp = 0.0f;
            for (int c = 0; c < numCh; ++c)
                for (int p = 0; p < OS; ++p)
                {
                    float acc = 0.0f; int idx = detPos;
                    const auto& ph = phase[(size_t) p];
                    for (int k = 0; k < tapsPerPhase; ++k)
                    {
                        acc += ph[(size_t) k] * detBuf[(size_t) c][(size_t) idx];
                        idx = (idx == 0) ? protoLen - 1 : idx - 1;
                    }
                    tp = std::max (tp, std::fabs (acc));
                }
            detPos = (detPos + 1) % protoLen;

            // 2b. smooth threshold/ceiling toward target (anti-zipper); derive makeup from the
            //     smoothed values so output peak stays == ceiling even mid-ramp.
            thrLin  += (thrTgt  - thrLin)  * smCoef;
            ceilLin += (ceilTgt - ceilLin) * smCoef;
            const float makeup = ceilLin / std::max (thrLin, 1.0e-6f);

            // 3. required gain to pull the true-peak down to threshold
            const float gr = (tp > thrLin) ? (thrLin / tp) : 1.0f;

            // 4. sliding minimum of gr over the lookahead window (monotonic deque)
            pushMin (gr);
            const float grWin = dqV[(size_t) dqHead];

            // 5. attack (snap down) / release (ramp up), clamped to the windowed min => ceiling guarantee
            if (grWin < gState) gState = grWin;
            else                gState += (grWin - gState) * relCoef;
            if (gState > grWin) gState = grWin;

            // 6. apply to the delayed audio * makeup
            const int rpos = (wpos - L + ring) % ring;
            for (int c = 0; c < numCh; ++c)
            {
                delay[(size_t) c][(size_t) wpos] = chans[c][i];             // write newest
                chans[c][i] = delay[(size_t) c][(size_t) rpos] * gState * makeup;   // read L back
            }
            wpos = (wpos + 1) % ring;
            minGain = std::min (minGain, gState);
        }
        grDb = -20.0f * std::log10 (std::max (minGain, 1.0e-6f));   // report peak GR this block
    }

private:
    static constexpr int OS = 8;    // detector oversampling (inter-sample-peak accuracy near Nyquist)

    static float db2lin (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
    static float juceClamp (float v, float lo, float hi) noexcept { return std::max (lo, std::min (hi, v)); }

    void buildDetector()
    {
        tapsPerPhase = 12;
        protoLen = tapsPerPhase * OS;                       // 8x * 12 = 96-tap prototype
        std::vector<float> h ((size_t) protoLen);
        const double PI = 3.14159265358979323846;
        const double fc = 0.5 / OS;                         // cutoff = original Nyquist
        const int M = protoLen - 1;
        double sum = 0.0;
        for (int m = 0; m < protoLen; ++m)
        {
            const double t = m - M / 2.0;
            const double s = (t == 0.0) ? 2.0 * fc : std::sin (2.0 * PI * fc * t) / (PI * t);
            const double w = 0.5 - 0.5 * std::cos (2.0 * PI * m / M);   // Hann
            h[(size_t) m] = (float) (s * w);
            sum += h[(size_t) m];
        }
        for (auto& v : h) v = (float) (v * OS / sum);        // unity gain per phase
        phase.assign ((size_t) OS, std::vector<float> ((size_t) tapsPerPhase, 0.0f));
        for (int m = 0; m < protoLen; ++m)
            phase[(size_t) (m % OS)][(size_t) (m / OS)] = h[(size_t) m];
    }

    // fixed-capacity monotonic deque -> sliding minimum over the last L pushes
    void pushMin (float v) noexcept
    {
        const int cap = (int) dqV.size();
        // pop larger-or-equal from the back
        while (dqSize > 0)
        {
            const int back = (dqHead + dqSize - 1) % cap;
            if (dqV[(size_t) back] >= v) --dqSize; else break;
        }
        const int slot = (dqHead + dqSize) % cap;
        dqV[(size_t) slot] = v; dqI[(size_t) slot] = nowIdx; ++dqSize;
        // drop front entries older than the window
        while (dqSize > 0 && dqI[(size_t) dqHead] <= nowIdx - L) { dqHead = (dqHead + 1) % cap; --dqSize; }
        ++nowIdx;
    }

    double sr = 48000.0; int nch = 2;
    float thrDb_ = -6.0f, ceilDb_ = -1.0f, relMs_ = 60.0f, lookMs_ = 1.5f;
    float thrLin = 0.5f, ceilLin = 0.88f, thrTgt = 0.5f, ceilTgt = 0.88f, relCoef = 0.01f, smCoef = 0.02f;
    int   L = 1, maxL = 1, ring = 2, detDelay = 0, protoLen = 0, tapsPerPhase = 8;

    std::vector<std::vector<float>> phase;   // [OS][tapsPerPhase]
    std::vector<std::vector<float>> detBuf;  // [nch][protoLen] ring
    std::vector<std::vector<float>> delay;   // [nch][ring] lookahead delay
    int detPos = 0, wpos = 0;

    std::vector<float> dqV; std::vector<long long> dqI;   // monotonic deque storage
    // nowIdx is 64-bit: at 48 kHz a 32-bit counter overflowed (signed UB) after ~12 h of continuous
    // playback, corrupting the sliding-min window; 64-bit pushes that past any realistic session.
    int dqHead = 0, dqSize = 0; long long nowIdx = 0;

    float gState = 1.0f, grDb = 0.0f;
};
