// Detail Forge — clipper engine (detail-preserving).
// Drive into a variable-ceiling clip; Knee morphs hard <-> soft; ADAA Order
// picks the anti-aliasing quality (Off / 1st / 2nd). Clipping happens at +/-Ceiling.
// Oversampling is applied in the JUCE layer around this engine.
//
// Detail preservation (the Au5 "detail-preserving clipper" / high-pass FOLDBACK technique):
//   residual = driven - clipped         // exactly the peaks the clipper chopped off
//   hf       = HF(residual)             // isolate the high-frequency part of those peaks
//   out      = clipped - Detail * hf    // FOLD that HF back down into the signal
// This is FOLDBACK, not addition: subtracting the HF residual reflects the peak back down
// (out = clip - residual = 2*clip - x when the filter passes all of it) instead of flattening
// it — the waveform's detail is preserved as a fold rather than snipped flat. The LF of the
// residual stays clipped (loudness/density); only the HF is folded (transient snap/air).
// DetailFreq sets how much folds: low cutoff = fold almost everything (dramatic), high = fold
// only the top sparkle. HF is COMPLEMENTARY (residual - lowpass(residual)) → phase-coherent.
//
// SAMPLE RATE: this engine is init()'d at the BASE rate but fed OVERSAMPLED blocks, so the
// JUCE layer pre-divides DetailFreq by the live oversampling factor before writing the zone
// — the bilinear prewarp makes that compensation exact (see PluginProcessor::processBlock).
//
//   MapUI paths: /DetailForgeClip/{Drive,Knee,Ceiling,Order,Detail,DetailFreq}
//
// Regenerate: faust -lang cpp -cn ClipEngine Source/dsp/clip.dsp -o <build>/ClipEngine.h

declare name "DetailForgeClip";
import("stdfaust.lib");

drive      = hslider("Drive[unit:dB]", 2.0, 0.0, 36.0, 0.01) : ba.db2linear;  // gentle default
knee       = hslider("Knee", 0.35, 0.0, 1.0, 0.001);                          // 0 hard, 1 soft
ceiling    = hslider("Ceiling[unit:dB]", -3.0, -18.0, 0.0, 0.01) : ba.db2linear;
order      = nentry("Order", 2, 0, 2, 1);                                     // 0 off, 1 first, 2 second
detail     = hslider("Detail", 0.0, 0.0, 2.0, 0.001);                         // fold-back amount (0 = off)
detailFreq = hslider("DetailFreq[unit:Hz][scale:log]", 3000.0, 20.0, 12000.0, 1.0); // fold split (pre-scaled by OS factor); low = fold more

// Unit clippers at each ADAA order (all map [-1,1] -> [-1,1]).
hardSel(x) = select3(order, max(-1.0, min(1.0, x)), aa.hardclip(x), aa.hardclip2(x));
softSel(x) = select3(order, aa.softclipEnv.softclip(x), aa.softclipQuadratic1(x), aa.softclipQuadratic2(x));
shaped(x)  = (1.0 - knee) * hardSel(x) + knee * softSel(x);

// Clip the driven signal at +/-Ceiling: scale so the unit clip acts at the ceiling, back up.
clipCore(x) = x : /(ceiling) : shaped : *(ceiling);

// Detail-preserving clip: FOLD the HF of the clipped-off residual back down (subtract, not add).
// Detail = 0 collapses this to clipCore exactly (back-compat with the plain clipper).
dpClip(x) = clipped - detail * hf
with {
    clipped  = clipCore(x);
    residual = x - clipped;                                    // the clipped-off peaks
    hf       = residual - (residual : fi.lowpass(2, detailFreq)); // phase-coherent HF
};

// Drive in, then the detail-preserving clip.
process = *(drive) : dpClip;
