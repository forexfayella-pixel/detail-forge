// Detail Forge — clipper engine.
// Drive into a variable-ceiling clip; Knee morphs hard <-> soft; ADAA Order
// picks the anti-aliasing quality (Off / 1st / 2nd). Clipping happens at +/-Ceiling.
// Oversampling is applied in the JUCE layer around this engine.
//
//   MapUI paths: /DetailForgeClip/{Drive,Knee,Ceiling,Order}
//
// Regenerate: faust -lang cpp -cn ClipEngine Source/dsp/clip.dsp -o <build>/ClipEngine.h

declare name "DetailForgeClip";
import("stdfaust.lib");

drive   = hslider("Drive[unit:dB]", 2.0, 0.0, 36.0, 0.01) : ba.db2linear;   // gentle default
knee    = hslider("Knee", 0.35, 0.0, 1.0, 0.001);                            // 0 hard, 1 soft
ceiling = hslider("Ceiling[unit:dB]", -3.0, -18.0, 0.0, 0.01) : ba.db2linear;
order   = nentry("Order", 2, 0, 2, 1);                                       // 0 off, 1 first, 2 second

// Unit clippers at each ADAA order (all map [-1,1] -> [-1,1]).
hardSel(x) = select3(order, max(-1.0, min(1.0, x)), aa.hardclip(x), aa.hardclip2(x));
softSel(x) = select3(order, aa.softclipEnv.softclip(x), aa.softclipQuadratic1(x), aa.softclipQuadratic2(x));
shaped(x)  = (1.0 - knee) * hardSel(x) + knee * softSel(x);

// Drive in, scale so the clip acts at +/-Ceiling, shape, scale back to Ceiling.
process = *(drive) : /(ceiling) : shaped : *(ceiling);
