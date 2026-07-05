#!/usr/bin/env python3
"""Detail Forge — M0 measurement analysis.

Reads CSV columns produced by the C++ engine-runner (Tests/engine_runner) and
prints the fidelity metrics the M0 spec gates on:

  - null      : residual (dB) of (a - b) for two runs that should be identical.
  - alias     : worst-case aliasing (dB below fundamental) for a pure-sine drive
                test — compares the ADAA2 engine against a naive hard-clip control.
  - thdn      : THD+N (dB) vs the fundamental.

Usage:
  python scripts/analyze.py alias --fs 48000 --f0 5000 in.csv out_adaa.csv out_naive.csv
  python scripts/analyze.py null  a.csv b.csv
  python scripts/analyze.py thdn  --fs 48000 --f0 1000 in.csv out.csv

CSV = one float sample per line (mono).
"""
import sys, argparse
import numpy as np


def load(path):
    return np.loadtxt(path, dtype=np.float64).ravel()


def db(x, floor=1e-20):
    return 20.0 * np.log10(np.maximum(np.abs(x), floor))


def spectrum(x):
    # Rectangular window. Intended for COHERENT sampling (f0 snapped to an exact
    # FFT bin) so the fundamental, harmonics, and their aliases all land exactly
    # on bins with negligible leakage — revealing the true alias floor.
    return np.abs(np.fft.rfft(x))


def coherent_freq(target, fs, n):
    """Snap a target frequency to the nearest exact FFT bin center."""
    return round(target * n / fs) * fs / n


def nearest_bin(fs, n, f):
    return int(round(f / fs * n))


def cmd_null(args):
    # Compare out (b) against in (a) shifted by --shift samples (the engine latency),
    # ignoring the first/last few samples (ADAA edge transient).
    a, b = load(args.a), load(args.b)
    s = args.shift
    n = min(len(a) - s, len(b))
    guard = 8
    ref = a[:n]
    got = b[s:s + n]
    resid = (got - ref)[guard:n - guard]
    rms = np.sqrt(np.mean(resid**2)) if len(resid) else 0.0
    rdb = 20 * np.log10(max(rms, 1e-20))
    print(f"null residual RMS: {rdb:.1f} dB (shift={s}, threshold <= {args.threshold} dB) "
          f"-> {'PASS' if rdb <= args.threshold else 'FAIL'}")


def cmd_latency(args):
    # Impulse response: latency = index of peak magnitude.
    x = load(args.out)
    idx = int(np.argmax(np.abs(x)))
    print(f"latency: {idx} samples (peak |value|={abs(x[idx]):.6g})")


def alias_level(x, fs, f0):
    """Worst-case non-harmonic energy (dB below fundamental)."""
    n = len(x)
    X = spectrum(x)
    freqs = np.fft.rfftfreq(n, 1.0 / fs)
    fund_bin = nearest_bin(fs, n, f0)
    fund = X[fund_bin] if fund_bin < len(X) else 0.0
    # Mask out harmonics (k*f0) +/- a few bins; the rest is alias+noise.
    mask = np.ones(len(X), dtype=bool)
    guard = 3
    k = 1
    while k * f0 < fs / 2:
        b = nearest_bin(fs, n, k * f0)
        mask[max(0, b - guard):b + guard + 1] = False
        k += 1
    mask[:guard + 1] = False  # ignore DC
    worst = np.max(X[mask]) if mask.any() else 0.0
    return 20 * np.log10(max(worst, 1e-20) / max(fund, 1e-20))


def cmd_alias(args):
    adaa = load(args.adaa)
    naive = load(args.naive)
    a_db = alias_level(adaa, args.fs, args.f0)
    n_db = alias_level(naive, args.fs, args.f0)
    print(f"alias (ADAA2):  {a_db:.1f} dB below fundamental")
    print(f"alias (naive):  {n_db:.1f} dB below fundamental")
    print(f"improvement:    {a_db - n_db:.1f} dB")
    ok = (a_db <= -args.threshold) and (a_db < n_db)
    print(f"gate: ADAA2 <= -{args.threshold} dB AND beats naive -> {'PASS' if ok else 'FAIL'}")


def cmd_thdn(args):
    x = load(args.out)
    n = len(x)
    X = spectrum(x)
    fund_bin = nearest_bin(fs=args.fs, n=n, f=args.f0)
    fund = X[fund_bin]
    total = np.sqrt(np.sum(X**2))
    guard = 3
    rest = X.copy()
    rest[max(0, fund_bin - guard):fund_bin + guard + 1] = 0.0
    thdn = np.sqrt(np.sum(rest**2)) / max(fund, 1e-20)
    print(f"THD+N: {20*np.log10(max(thdn,1e-20)):.1f} dB (fundamental {args.f0} Hz)")


def main():
    p = argparse.ArgumentParser()
    sub = p.add_subparsers(dest="cmd", required=True)

    pn = sub.add_parser("null"); pn.add_argument("a"); pn.add_argument("b")
    pn.add_argument("--shift", type=int, default=0)
    pn.add_argument("--threshold", type=float, default=-120.0); pn.set_defaults(fn=cmd_null)

    pl = sub.add_parser("latency"); pl.add_argument("out"); pl.set_defaults(fn=cmd_latency)

    pa = sub.add_parser("alias"); pa.add_argument("adaa"); pa.add_argument("naive")
    pa.add_argument("--fs", type=float, default=48000); pa.add_argument("--f0", type=float, default=5000)
    pa.add_argument("--threshold", type=float, default=60.0); pa.set_defaults(fn=cmd_alias)

    pt = sub.add_parser("thdn"); pt.add_argument("out")
    pt.add_argument("--fs", type=float, default=48000); pt.add_argument("--f0", type=float, default=1000)
    pt.set_defaults(fn=cmd_thdn)

    args = p.parse_args()
    args.fn(args)


if __name__ == "__main__":
    main()
