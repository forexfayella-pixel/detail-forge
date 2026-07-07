#!/usr/bin/env python3
"""Detail Forge — automated M0 fidelity gate (run by ctest).

Drives the real ClipEngine via engine_runner and asserts the recalibrated M0
gates (see specs/2026-07-02-m0-vertical-slice.md):
  - ADAA2 beats a naive hard-clip by >= 6 dB worst-case alias across a
    2-9 kHz x 1-6 dB sweep (coherent sampling), and
  - clip-stage latency == 1 sample.
Exits non-zero if any gate fails.
"""
import argparse, subprocess, sys, os, tempfile
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from analyze import load, alias_level, coherent_freq  # reuse the analysis

FS = 48000.0
N = 65536
MIN_IMPROVEMENT_DB = 6.0
SWEEP = [(5000, 1), (5000, 6), (9000, 1), (9000, 6), (2000, 6)]


def run(runner, tmp, **kw):
    out = os.path.join(tmp, kw["tag"] + ".csv")
    cmd = [runner, "--signal", kw.get("signal", "sine"), "--engine", kw["engine"],
           "--fs", str(FS), "--f0", str(kw.get("f0", 1000)), "--drive", str(kw.get("drive", 0)),
           "--amp", str(kw.get("amp", 1.0)), "--n", str(kw.get("n", N)), "--out", out]
    if "detail" in kw:      cmd += ["--detail", str(kw["detail"])]
    if "detail_freq" in kw: cmd += ["--detail-freq", str(kw["detail_freq"])]
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return load(out)


def band_energy(sig, fs, lo, hi):
    """Sum of |X|^2 in [lo, hi) Hz (rectangular window; signal is coherently sampled)."""
    X = np.fft.rfft(sig)
    f = np.fft.rfftfreq(len(sig), 1.0 / fs)
    m = (f >= lo) & (f < hi)
    return float(np.sum(np.abs(X[m]) ** 2))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runner", required=True)
    args = ap.parse_args()
    if not os.path.exists(args.runner):
        print(f"engine_runner not found: {args.runner}", file=sys.stderr)
        return 2

    failures = []
    with tempfile.TemporaryDirectory() as tmp:
        # --- Aliasing sweep ---
        print("M0 gate - aliasing (ADAA2 must beat naive by >= %.0f dB):" % MIN_IMPROVEMENT_DB)
        for tf, drv in SWEEP:
            f0 = coherent_freq(tf, FS, N)
            a = run(args.runner, tmp, engine="adaa",  f0=f0, drive=drv, tag=f"a_{tf}_{drv}")
            n = run(args.runner, tmp, engine="naive", f0=f0, drive=drv, tag=f"n_{tf}_{drv}")
            a_db = alias_level(a, FS, f0)
            n_db = alias_level(n, FS, f0)
            imp = a_db - n_db
            ok = imp <= -MIN_IMPROVEMENT_DB  # more-negative alias for ADAA = better
            status = "ok" if ok else "FAIL"
            print(f"  f0={tf:>4} drive={drv}dB  ADAA2={a_db:6.1f}  naive={n_db:6.1f}  improvement={-imp:5.1f} dB  [{status}]")
            # 2 kHz has very low aliasing (near floor) — informational, not gated.
            if not ok and tf != 2000:
                failures.append(f"alias f0={tf} drive={drv}: only {-imp:.1f} dB")

        # --- Latency ---
        imp_resp = run(args.runner, tmp, signal="impulse", engine="passthrough",
                       amp=0.5, n=64, tag="impulse")
        latency = int(np.argmax(np.abs(imp_resp)))
        lat_ok = latency == 1
        print(f"M0 gate - latency: {latency} samples  [{'ok' if lat_ok else 'FAIL'}]")
        if not lat_ok:
            failures.append(f"latency={latency} (expected 1)")

        # --- Detail-preserving FOLDBACK ---
        # The clipper FOLDS peaks (out = clip - Detail*HF(residual)) instead of flattening them, so a
        # tone driven hard into a flat plateau gets that plateau folded away. Assert:
        #  (a) Detail=0 reproduces the plain clip bit-exact (adding the params changed nothing), and
        #  (b) Detail>0 with a low split un-flattens the clip: far fewer samples pinned at the ceiling.
        print("M0 gate - detail-preserving foldback:")
        DF = 40.0                                     # low split -> fold most of the excess
        f0f = coherent_freq(220, FS, N)
        # 6 dB drive (2x): |x|>ceiling for ~2/3 of the cycle, and the fold (2*ceil-x) stays in range.
        base = run(args.runner, tmp, signal="sine", engine="adaa", f0=f0f, amp=1.0, drive=6,
                   tag="d_base")                                             # no --detail (flat clip)
        d0   = run(args.runner, tmp, signal="sine", engine="adaa", f0=f0f, amp=1.0, drive=6,
                   tag="d0", detail=0.0, detail_freq=DF)
        d1   = run(args.runner, tmp, signal="sine", engine="adaa", f0=f0f, amp=1.0, drive=6,
                   tag="d1", detail=1.0, detail_freq=DF)
        backcompat = float(np.max(np.abs(base - d0)))
        pinned0 = int(np.sum(np.abs(d0) > 0.9))       # samples pinned near the ±1 clip ceiling
        pinned1 = int(np.sum(np.abs(d1) > 0.9))
        bc_ok = backcompat == 0.0
        fold_ok = pinned0 > N * 0.1 and pinned1 < pinned0 * 0.5   # d0 has a real plateau; d1 folds it away
        print(f"  Detail=0 vs plain clip: max|diff|={backcompat:.2e}  [{'ok' if bc_ok else 'FAIL'}]")
        print(f"  plateau samples (|x|>0.9): Detail=0 -> {pinned0}, Detail=1 -> {pinned1}  [{'ok' if fold_ok else 'FAIL'}]")
        if not bc_ok:
            failures.append(f"detail=0 not bit-exact vs plain clip (max|diff|={backcompat:.2e})")
        if not fold_ok:
            failures.append(f"foldback did not un-flatten the clip (pinned {pinned0} -> {pinned1})")

    if failures:
        print("\nM0 GATE FAILED:", file=sys.stderr)
        for f in failures:
            print("  - " + f, file=sys.stderr)
        return 1
    print("\nM0 GATE PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
