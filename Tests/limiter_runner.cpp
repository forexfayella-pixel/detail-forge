// Detail Forge — M2 limiter gate (no JUCE). Drives the real TruePeakLimiter over engineered
// signals and asserts the spec's core guarantee: the OUTPUT true-peak never exceeds the ceiling —
// including a near-Nyquist inter-sample-peak signal (sample peaks low, true peak high) — while the
// limiter actually engages gain reduction. Independent 8x-sinc true-peak measurement of the output
// (a different filter than the limiter's internal detector, so it's a real check, not circular).
//
// Exit 0 = all cases pass; 1 = a ceiling breach or no-GR case. Wired into ctest as `m2_gate`.
#include <cstdio>
#include <cmath>
#include <vector>
#include "dsp/Limiter.h"

static const double PI = 3.14159265358979323846;

// Independent 8x windowed-sinc measurement of a signal's true (inter-sample) peak from `start`.
static double truePeak8x (const std::vector<float>& x, int start)
{
    const int OS = 8, P = 12, N = P * OS, M = N - 1; const double fc = 0.5 / OS;
    std::vector<double> h (N); double sum = 0;
    for (int m = 0; m < N; ++m) { double t = m - M / 2.0;
        double s = (t == 0) ? 2 * fc : std::sin (2 * PI * fc * t) / (PI * t);
        double w = 0.5 - 0.5 * std::cos (2 * PI * m / M); h[m] = s * w; sum += h[m]; }
    for (auto& v : h) v *= OS / sum;
    double pk = 0;
    for (int n = start; n < (int) x.size(); ++n)
        for (int p = 0; p < OS; ++p) { double acc = 0;
            for (int k = 0; k < P; ++k) { int idx = n - k; if (idx >= 0) acc += h[p + k * OS] * x[idx]; }
            pk = std::max (pk, std::fabs (acc)); }
    return pk;
}
static double db2lin (double db) { return std::pow (10.0, db * 0.05); }

int main()
{
    const double sr = 48000, ceil = -1.0, thr = -6.0; const int N = 8192;
    struct Case { const char* name; double amp, freq, look; } cases[] = {
        { "hot 1kHz",         0.95, 1000,  2.0 },
        { "near-Nyquist ISP", 0.90, 21000, 2.0 },   // inter-sample peaks >> sample peaks
        { "very hot low",     1.20, 120,   4.0 },
        { "zero lookahead",   0.95, 1000,  0.0 },
    };
    const double tol = db2lin (ceil) * db2lin (0.3);   // allow +0.3 dB over the nominal ceiling
    bool ok = true;
    printf ("M2 gate - true-peak ceiling hold (ceiling %.1f dBTP):\n", ceil);
    for (auto& c : cases) {
        TruePeakLimiter lim; lim.prepare (sr, 2);
        lim.setParams ((float) thr, (float) ceil, 60.0f, (float) c.look);
        std::vector<float> L (N), R (N);
        for (int n = 0; n < N; ++n) { float v = (float) (c.amp * std::sin (2 * PI * c.freq * n / sr)); L[n] = v; R[n] = v; }
        for (int n = 0; n < N; ++n) { float* ch[2] = { &L[n], &R[n] }; lim.process (ch, 2, 1); }
        const double outTP = truePeak8x (L, lim.getLatencySamples() + 64);
        const double grDb  = lim.getGainReductionDb();
        const bool held = outTP <= tol, eng = grDb > 0.5, pass = held && eng;
        ok = ok && pass;
        printf ("  %-18s outTP=%.2f dBTP  GR=%.2f dB  [%s]\n",
                c.name, 20 * std::log10 (std::max (outTP, 1e-9)), grDb, pass ? "ok" : (held ? "NO-GR" : "OVER"));
    }
    printf (ok ? "\nM2 GATE PASSED\n" : "\nM2 GATE FAILED\n");
    return ok ? 0 : 1;
}
