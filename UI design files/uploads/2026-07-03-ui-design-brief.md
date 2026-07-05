# Detail Forge — UI Design Brief (for the Design stage)

Paste the block below into Design to generate UI visuals. Then bring the result back and I'll wire it to the live parameters.

---

## THE PLUGIN (identity)
**Detail Forge** is a *detail-preserving* dynamics processor — a single-band **Saturator → Clipper → Limiter** chain that makes a track louder, denser, and more characterful **without the harshness or smearing** you normally pay for. Its whole reason to exist: anti-aliasing (ADAA) runs through every nonlinear stage, and the clip→limit topology is true-peak-aware, so it stays clean and detailed even when pushed hard. Think **precision mastering-grade instrument**, not a toy.

The name carries a productive tension the design should fuse:
- **"Forge"** — heat, metal, molten amber, craftsmanship, power.
- **"Detail"** — clarity, precision, transparency, surgical control.
→ A **dark, precise instrument face** with **warm molten-amber accents** for the drive/heat controls and **cool, crisp light** (near-white / faint cyan) for metering and detail readouts.

## SIGNAL FLOW (the mental model the layout should convey)
Audio flows **left → right** through three stages, framed by global I/O:

`INPUT → [ SATURATOR ] → [ CLIPPER ] → [ LIMITER ] → OUTPUT`

- **Saturator** — adds harmonic density & character (the "warmth").
- **Clipper** — catches transient peaks per-sample, anti-aliased (the "punch, made clean").
- **Limiter** — lookahead true-peak brickwall (the "safe ceiling / loudness").

A left-to-right three-panel layout that mirrors this flow is ideal.

## FULL CONTROL SURFACE (functions & utilities)
Design for the complete set below. (Only **Drive** is live today; the rest arrive over the next builds — designing the whole surface now keeps the visual system consistent.) Each control lists its **stable id** (must appear so I can bind it), range, and what it does.

### 1 · SATURATOR  (warm harmonic front-end)
- **Voicing** `sat_voicing` — 3-way selector: **Tube / Tape / Transformer**. Changes the harmonic character (tube = asymmetric/even, tape = smooth/odd, transformer = punchy/polynomial).
- **Drive** `sat_drive` — 0…+24 dB. How hard the signal hits the saturator.
- **Bias** `sat_bias` — 0…100%. Adds even harmonics / asymmetry (0 = pure/symmetric).
- **Mix** `sat_mix` — 0…100% dry/wet. Parallel blend — the dry path carries the detail.

### 2 · CLIPPER  (anti-aliased transient control) — the current heart of the plugin
- **Drive** `clip_drive` — 0…+36 dB. *(LIVE NOW.)* Push into the clip ceiling.
- **Knee** `clip_knee` — Hard ↔ Soft morph. Hard = aggressive/edgy, soft = rounded.
- **Ceiling / Threshold** `clip_ceiling` — −18…0 dB. Where clipping begins.
- **Oversampling** `oversampling` — selector: **1× / 2× / 4× / 8× / 16×**. Higher = cleaner (less aliasing), more CPU.
- **ADAA Order** `adaa_order` — selector: **Off / 1st / 2nd**. Anti-aliasing quality vs CPU.

### 3 · LIMITER  (lookahead, true-peak brickwall)
- **Threshold** `lim_threshold` — −24…0 dB. Where limiting kicks in (drives loudness).
- **Ceiling** `lim_ceiling` — −2…0 dB. True-peak output ceiling (default −1.0, strict −1.2).
- **Release** `lim_release` — 1…500 ms. How fast gain recovers.

### 4 · GLOBAL
- **Input** `in_gain` — −12…+12 dB.
- **Output** `out_gain` — −12…+12 dB.
- **Bypass** `bypass` — toggle (true bypass).
- **Presets** — a preset name display + prev/next + save/load (browse a small library).

## METERING & FEEDBACK (please design these — they sell the "detail" promise)
- **Input & Output level meters** — vertical, with peak-hold ticks.
- **Gain-Reduction meter** — for the limiter (how much it's pulling down).
- **True-Peak / Over indicator** — a small LED/readout that flags inter-sample overs.
- **(Bonus, optional)** a live **transfer-curve** or **harmonic spectrum** thumbnail — a small graphic that visualizes what the clipper/saturator is doing. High-impact for a "detail" tool.

## VISUAL / BRAND DIRECTION
- **Dark-first** instrument face (deep charcoal / near-black, subtle depth/texture — brushed metal or matte).
- **Accent = molten amber/orange** (heat) for drive-type controls; **cool near-white / faint cyan** for meters and numeric readouts. Keep it restrained and premium — not neon, not skeuomorphic-cheesy.
- **Typography**: clean geometric/technical sans; **tabular numerals** for all value readouts.
- **Controls**: tactile rotary knobs for continuous params; clear segmented switches for selectors (voicing / oversampling / ADAA order); a prominent bypass.
- **Feel**: a serious, modern mastering instrument — confident, legible, a little industrial warmth.

## PRACTICAL CONSTRAINTS (so the result is wire-able)
This is a **JUCE 8 WebView UI = HTML/CSS/JS**, so:
- **Fully self-contained** — no external CDNs, web fonts, or remote images. Everything inline or embeddable (fonts as data-URI or system stack; art as inline SVG or embedded).
- **Resizable & responsive** — use relative units; the window scales. Design at a **default of ~760 × 460 px** but ensure it scales gracefully (roughly 1.6:1). No fixed-pixel-only layouts.
- **Each interactive control must be identifiable** by the **stable id** listed above (a `data-` attribute, element id, or clearly labeled in the mockup) so I can bind it to the DSP via JUCE's `getSliderState(id)` / `getComboBoxState(id)` / `getToggleState(id)`.
- **Ideal deliverable**: an **HTML/CSS mockup** (I wire the JS bindings on top). If Design outputs **images/mockups only**, that's fine — I'll rebuild it faithfully as HTML/CSS. Either way, please also provide the **palette (hex), type choices, and spacing** as tokens so it stays consistent.

## DELIVERABLES REQUESTED
1. A **full-window mockup** of the default state (all controls, meters, signal-flow layout).
2. The **brand tokens**: color palette (hex), typography, spacing scale, corner radii.
3. Individual **control artwork** if custom (knob, switch, meter, bypass) — or note if CSS-drawn.
4. (Optional) an **"engaged" state** mockup (metering active, gain reduction showing) to define the live look.

---
*Note for wiring: today only `clip_drive` exists in the DSP (0–36 dB). I'll bind controls as each stage's DSP lands (Clipper → M1, Limiter → M2, Saturator → M3, full metering/presets → M4). Designing the whole face now means the UI just gets progressively "activated."*
