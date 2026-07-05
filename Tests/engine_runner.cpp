// Detail Forge — M0 measurement runner (no JUCE).
// Drives the *real* Faust ClipEngine (or a naive hard-clip control) over a test
// signal and writes the output as one float sample per line. Paired with
// scripts/analyze.py for null / aliasing / THD+N metrics and latency.
//
// Usage:
//   engine_runner --signal sine|impulse --engine adaa|naive|passthrough \
//                 --fs 48000 --f0 5000 --drive 24 --amp 1.0 --n 65536 \
//                 --out out.csv [--emit-input in.csv]

#define _USE_MATH_DEFINES
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "faust/gui/meta.h"
#include "faust/gui/MapUI.h"
#include "faust/dsp/dsp.h"
#include "ClipEngine.h"

static const char* kDrivePath   = "/DetailForgeClip/Drive";
static const char* kKneePath    = "/DetailForgeClip/Knee";
static const char* kCeilingPath  = "/DetailForgeClip/Ceiling";
static const char* kOrderPath    = "/DetailForgeClip/Order";

static double argd(int argc, char** argv, const char* key, double def) {
    for (int i = 1; i + 1 < argc; ++i) if (!std::strcmp(argv[i], key)) return std::atof(argv[i + 1]);
    return def;
}
static std::string args(int argc, char** argv, const char* key, const char* def) {
    for (int i = 1; i + 1 < argc; ++i) if (!std::strcmp(argv[i], key)) return argv[i + 1];
    return def;
}
static bool writeCsv(const std::string& path, const std::vector<float>& v) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) { std::fprintf(stderr, "cannot open %s\n", path.c_str()); return false; }
    for (float x : v) std::fprintf(f, "%.10g\n", x);
    std::fclose(f);
    return true;
}

int main(int argc, char** argv) {
    const std::string signal = args(argc, argv, "--signal", "sine");
    const std::string engine = args(argc, argv, "--engine", "adaa");
    const double fs   = argd(argc, argv, "--fs", 48000.0);
    const double f0   = argd(argc, argv, "--f0", 5000.0);
    const double drive= argd(argc, argv, "--drive", 24.0);   // dB
    const double amp  = argd(argc, argv, "--amp", 1.0);
    const int    N    = (int) argd(argc, argv, "--n", 65536);
    const std::string outPath = args(argc, argv, "--out", "out.csv");
    const std::string inPath  = args(argc, argv, "--emit-input", "");

    // Build the input signal.
    std::vector<float> in(N, 0.0f), out(N, 0.0f);
    if (signal == "impulse") {
        in[0] = (float) amp;
    } else if (signal == "ramp") { // transfer-curve probe: x in [-1,1] → out is y(x)
        for (int i = 0; i < N; ++i) in[i] = (float) (-1.0 + 2.0 * i / (N - 1));
    } else { // sine
        const double w = 2.0 * M_PI * f0 / fs;
        for (int i = 0; i < N; ++i) in[i] = (float) (amp * std::sin(w * i));
    }

    if (engine == "naive") {
        // Drive then instantaneous hard clip (aliasing control).
        const double g = std::pow(10.0, drive / 20.0);
        for (int i = 0; i < N; ++i) {
            double x = g * in[i];
            out[i] = (float) (x > 1.0 ? 1.0 : (x < -1.0 ? -1.0 : x));
        }
    } else {
        // Real Faust engine (ADAA2 hard clip). "passthrough" = drive 0 dB.
        ClipEngine dsp;
        dsp.init((int) fs);
        MapUI ui;
        dsp.buildUserInterface(&ui);
        const double d = (engine == "passthrough") ? 0.0 : drive;
        ui.setParamValue(kDrivePath,   (FAUSTFLOAT) d);
        ui.setParamValue(kKneePath,    (FAUSTFLOAT) argd(argc, argv, "--knee", 0.0));      // 0 = hard, isolate AA
        ui.setParamValue(kCeilingPath, (FAUSTFLOAT) argd(argc, argv, "--ceiling", 0.0));   // dB, 0 = clip at +/-1
        ui.setParamValue(kOrderPath,   (FAUSTFLOAT) argd(argc, argv, "--order", 2.0));     // 0 off,1 first,2 second
        if (signal == "ramp") {
            // Mirror DetailForgeProcessor::computeTransferCurve: warm up at x=-1, then the ramp,
            // and keep only the ramp portion — so this is a faithful ground truth of the probe.
            const int W = 16;
            std::vector<float> b(W + N, -1.0f);
            for (int i = 0; i < N; ++i) b[W + i] = in[i];
            float* bp[1] = { b.data() };
            dsp.compute(W + N, bp, bp);
            for (int i = 0; i < N; ++i) out[i] = b[W + i];
        } else {
            float* ins[1]  = { in.data() };
            float* outs[1] = { out.data() };
            dsp.compute(N, ins, outs);
        }
    }

    if (!writeCsv(outPath, out)) return 1;
    if (!inPath.empty() && !writeCsv(inPath, in)) return 1;
    std::fprintf(stderr, "engine_runner: signal=%s engine=%s fs=%.0f f0=%.0f drive=%.1f N=%d -> %s\n",
                 signal.c_str(), engine.c_str(), fs, f0, drive, N, outPath.c_str());
    return 0;
}
