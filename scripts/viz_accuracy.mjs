// Accuracy reference for the waveform/true-peak visualization (Task 13 of the detail-viz plan).
//   node scripts/viz_accuracy.mjs
// Generates the signals the display must render truthfully and prints the reference numbers:
//   (1) inter-sample-peak signals: sample peak vs BS.1770 4x true-peak vs 8x reconstruction.
//   (2) an impulse for pre/post alignment (used with engine_runner / the live scope).
// The DISPLAY (8x Lanczos) and the METER (JUCE 4x) must land on these true-peak values, NOT the
// (lower) sample peak. If they show the sample peak, the display is lying about inter-sample peaks.

const dbfs = a => 20*Math.log10(a);

// --- BS.1770-style 4x true-peak: zero-stuff x4, windowed-sinc LPF at Nyquist, max|.| ---
function makeLPF(taps, os){                    // anti-imaging cutoff = original Nyquist = 0.5/os
  const fc=0.5/os, h=new Float64Array(taps), c=(taps-1)/2;
  let s=0;
  for(let i=0;i<taps;i++){ const n=i-c;
    const sinc = n===0 ? 2*fc : Math.sin(2*Math.PI*fc*n)/(Math.PI*n);
    const win = 0.5-0.5*Math.cos(2*Math.PI*i/(taps-1));   // Hann
    h[i]=sinc*win; s+=h[i];
  }
  for(let i=0;i<taps;i++) h[i]=h[i]/s*os;      // unity passband gain after zero-stuff
  return h;
}
function truePeak4x(x){
  const os=4, h=makeLPF(64,os), up=new Float64Array(x.length*os);
  for(let i=0;i<x.length;i++) up[i*os]=x[i];   // zero-stuff
  let pk=0;
  for(let n=0;n<up.length;n++){ let acc=0;
    for(let k=0;k<h.length;k++){ const j=n-k; if(j>=0&&j<up.length) acc+=up[j]*h[k]; }
    pk=Math.max(pk,Math.abs(acc));
  }
  return pk;
}
// --- 8x Lanczos reconstruction (the DISPLAY path) ---
function makeLanczos(a,os){const sinc=x=>{if(Math.abs(x)<1e-8)return 1;const px=Math.PI*x;return Math.sin(px)/px;};const L=t=>(Math.abs(t)<a)?sinc(t)*sinc(t/a):0;const taps=2*a,table=[];for(let p=0;p<os;p++){const frac=p/os,row=new Float64Array(taps);let s=0;for(let t=0;t<taps;t++){const c=L(frac+a-1-t);row[t]=c;s+=c;}if(s)for(let t=0;t<taps;t++)row[t]/=s;table.push(row);}return{a,os,taps,table};}
function reconPeak(x,k){const{a,os,taps,table}=k,N=x.length,at=i=>x[i<0?0:(i>=N?N-1:i)];let pk=0;for(let i=0;i<N;i++)for(let p=0;p<os;p++){let acc=0;for(let t=0;t<taps;t++)acc+=at(i-a+1+t)*table[p][t];pk=Math.max(pk,Math.abs(acc));}return pk;}
const LANC=makeLanczos(3,8);

function report(name, x){
  let sp=0; for(const v of x) sp=Math.max(sp,Math.abs(v));
  const tp=truePeak4x(x), rp=reconPeak(x,LANC);
  console.log(`${name}`);
  console.log(`   sample peak = ${sp.toFixed(4)} (${dbfs(sp).toFixed(2)} dBFS)`);
  console.log(`   true peak 4x (BS.1770 ref) = ${tp.toFixed(4)} (${dbfs(tp).toFixed(2)} dBTP)`);
  console.log(`   8x reconstruction (display) = ${rp.toFixed(4)} (${dbfs(rp).toFixed(2)} dB)`);
  console.log(`   inter-sample overshoot vs sample peak = +${dbfs(tp/sp).toFixed(2)} dB`);
  console.log(`   ${tp>sp*1.03 ? "OK — true peak exceeds sample peak (must be visible)" : "no meaningful inter-sample peak"}`);
}

const sr=48000, N=256;
// fs/4 sine at 45deg: samples land on +/-0.707, reconstructed peak = 1.0 (a classic ISP case)
const a=new Float32Array(N); for(let i=0;i<N;i++) a[i]=Math.sin(2*Math.PI*(sr/4)*i/sr + Math.PI/4);
report("(1) fs/4 sine, 45deg phase  [expect TP ~ 0 dBTP, sample ~ -3 dBFS]", a);
// fs/3 full-scale sine: also overshoots between samples
const b=new Float32Array(N); for(let i=0;i<N;i++) b[i]=Math.sin(2*Math.PI*(sr/3)*i/sr);
report("(2) fs/3 sine, full-scale", b);
console.log("(3) impulse: unit sample at index 0 — feed the plugin, confirm dry & wet impulses");
console.log("    land on the SAME x in the scope (and stay aligned when the OS factor changes).");
