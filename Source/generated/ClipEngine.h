/* ------------------------------------------------------------
name: "DetailForgeClip"
Code generated with Faust 2.85.9 (https://faust.grame.fr)
Compilation options: -lang cpp -fpga-mem-th 4 -ct 1 -cn ClipEngine -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __ClipEngine_H__
#define  __ClipEngine_H__

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <cstdint>

#ifndef FAUSTCLASS 
#define FAUSTCLASS ClipEngine
#endif

#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

#if defined(_WIN32)
#define RESTRICT __restrict
#else
#define RESTRICT __restrict__
#endif

static float ClipEngine_faustpower2_f(float value) {
	return value * value;
}
static float ClipEngine_faustpower3_f(float value) {
	return value * value * value;
}
static float ClipEngine_faustpower4_f(float value) {
	return value * value * value * value;
}

class ClipEngine : public dsp {
	
 private:
	
	FAUSTFLOAT fEntry0;
	FAUSTFLOAT fHslider0;
	FAUSTFLOAT fHslider1;
	float fVec0[3];
	FAUSTFLOAT fHslider2;
	int fSampleRate;
	
 public:
	ClipEngine() {
	}
	
	ClipEngine(const ClipEngine&) = default;
	
	virtual ~ClipEngine() = default;
	
	ClipEngine& operator=(const ClipEngine&) = default;
	
	void metadata(Meta* m) { 
		m->declare("aanl.lib/ADAA1:author", "Dario Sanfilippo");
		m->declare("aanl.lib/ADAA1:copyright", "Copyright (C) 2021 Dario Sanfilippo     <sanfilippo.dario@gmail.com>");
		m->declare("aanl.lib/ADAA1:license", "MIT License");
		m->declare("aanl.lib/ADAA2:author", "Dario Sanfilippo");
		m->declare("aanl.lib/ADAA2:copyright", "Copyright (C) 2021 Dario Sanfilippo     <sanfilippo.dario@gmail.com>");
		m->declare("aanl.lib/ADAA2:license", "MIT License");
		m->declare("aanl.lib/hardclip2:author", "Dario Sanfilippo");
		m->declare("aanl.lib/hardclip2:copyright", "Copyright (C) 2021 Dario Sanfilippo     <sanfilippo.dario@gmail.com>");
		m->declare("aanl.lib/hardclip2:license", "MIT License");
		m->declare("aanl.lib/hardclip:author", "Dario Sanfilippo");
		m->declare("aanl.lib/hardclip:copyright", "Copyright (C) 2021 Dario Sanfilippo     <sanfilippo.dario@gmail.com>");
		m->declare("aanl.lib/hardclip:license", "MIT License");
		m->declare("aanl.lib/name", "Faust Antialiased Nonlinearities");
		m->declare("aanl.lib/softclipQuadratic1:author", "David Braun");
		m->declare("aanl.lib/softclipQuadratic1:copyright", "Copyright (C) 2024 David Braun");
		m->declare("aanl.lib/softclipQuadratic1:license", "MIT license");
		m->declare("aanl.lib/softclipQuadratic2:author", "David Braun");
		m->declare("aanl.lib/softclipQuadratic2:copyright", "Copyright (C) 2024 David Braun");
		m->declare("aanl.lib/softclipQuadratic2:license", "MIT license");
		m->declare("aanl.lib/version", "1.4.2");
		m->declare("basics.lib/ifNc:author", "Oleg Nesterov");
		m->declare("basics.lib/ifNc:copyright", "Copyright (C) 2023 Oleg Nesterov <oleg@redhat.com>");
		m->declare("basics.lib/ifNc:license", "MIT-style STK-4.3 license");
		m->declare("basics.lib/ifNcNo:author", "Oleg Nesterov");
		m->declare("basics.lib/ifNcNo:copyright", "Copyright (C) 2023 Oleg Nesterov <oleg@redhat.com>");
		m->declare("basics.lib/ifNcNo:license", "MIT-style STK-4.3 license");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/version", "1.22.0");
		m->declare("compile_options", "-lang cpp -fpga-mem-th 4 -ct 1 -cn ClipEngine -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("filename", "clip.dsp");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.9.0");
		m->declare("name", "DetailForgeClip");
		m->declare("signals.lib/name", "Faust Routing Library");
		m->declare("signals.lib/version", "1.6.0");
	}

	virtual int getNumInputs() {
		return 1;
	}
	virtual int getNumOutputs() {
		return 1;
	}
	
	static void classInit(int sample_rate) {
	}
	
	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
	}
	
	virtual void instanceResetUserInterface() {
		fEntry0 = static_cast<FAUSTFLOAT>(2.0f);
		fHslider0 = static_cast<FAUSTFLOAT>(-3.0f);
		fHslider1 = static_cast<FAUSTFLOAT>(2.0f);
		fHslider2 = static_cast<FAUSTFLOAT>(0.35f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 3; l0 = l0 + 1) {
			fVec0[l0] = 0.0f;
		}
	}
	
	virtual void init(int sample_rate) {
		classInit(sample_rate);
		instanceInit(sample_rate);
	}
	
	virtual void instanceInit(int sample_rate) {
		instanceConstants(sample_rate);
		instanceResetUserInterface();
		instanceClear();
	}
	
	virtual ClipEngine* clone() {
		return new ClipEngine(*this);
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("DetailForgeClip");
		ui_interface->declare(&fHslider0, "unit", "dB");
		ui_interface->addHorizontalSlider("Ceiling", &fHslider0, FAUSTFLOAT(-3.0f), FAUSTFLOAT(-18.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(0.01f));
		ui_interface->declare(&fHslider1, "unit", "dB");
		ui_interface->addHorizontalSlider("Drive", &fHslider1, FAUSTFLOAT(2.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(36.0f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("Knee", &fHslider2, FAUSTFLOAT(0.35f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.001f));
		ui_interface->addNumEntry("Order", &fEntry0, FAUSTFLOAT(2.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(2.0f), FAUSTFLOAT(1.0f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* output0 = outputs[0];
		float fSlow0 = static_cast<float>(fEntry0);
		int iSlow1 = fSlow0 == 0.0f;
		int iSlow2 = fSlow0 == 1.0f;
		float fSlow3 = std::pow(1e+01f, 0.05f * static_cast<float>(fHslider0));
		float fSlow4 = std::pow(1e+01f, 0.05f * static_cast<float>(fHslider1)) / fSlow3;
		float fSlow5 = 2.0f * fSlow4;
		float fSlow6 = static_cast<float>(fHslider2);
		float fSlow7 = 1.0f - fSlow6;
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			float fTemp0 = static_cast<float>(input0[i0]);
			float fTemp1 = fSlow4 * fTemp0;
			fVec0[0] = fTemp1;
			int iTemp2 = ClipEngine_faustpower2_f(fVec0[1] - fVec0[2]) <= 0.001f;
			float fTemp3 = ClipEngine_faustpower2_f(fVec0[2] - fVec0[1]);
			float fTemp4 = std::fabs(fVec0[2]);
			int iTemp5 = fTemp4 < 0.33333334f;
			int iTemp6 = fTemp4 < 0.6666667f;
			float fTemp7 = ClipEngine_faustpower2_f(fVec0[2]);
			float fTemp8 = 0.5f * fTemp7;
			int iTemp9 = fVec0[2] > 0.0f;
			float fTemp10 = static_cast<float>(((iTemp9) ? 1 : -1));
			float fTemp11 = ClipEngine_faustpower3_f(fVec0[2]);
			float fTemp12 = std::fabs(fVec0[1]);
			int iTemp13 = fTemp12 < 0.33333334f;
			int iTemp14 = fTemp12 < 0.6666667f;
			float fTemp15 = ClipEngine_faustpower2_f(fVec0[1]);
			float fTemp16 = ((iTemp13) ? fTemp15 : ((iTemp14) ? 2.0f * fTemp15 + (0.037037037f - (0.33333334f * fTemp12 + ClipEngine_faustpower3_f(fTemp12))) : fTemp12 + -0.25925925f));
			float fTemp17 = 0.5f * fTemp15;
			int iTemp18 = fVec0[1] > 0.0f;
			float fTemp19 = static_cast<float>(((iTemp18) ? 1 : -1));
			float fTemp20 = ClipEngine_faustpower3_f(fVec0[1]);
			float fTemp21 = ((iTemp13) ? 0.6666667f * fTemp20 : ((iTemp14) ? fTemp19 * (0.0030864198f - (0.16666667f * fTemp15 + 0.75f * ClipEngine_faustpower4_f(fVec0[1]))) + 1.3333334f * fTemp20 : fTemp19 * (fTemp17 + -0.046296295f)));
			float fTemp22 = 2.0f * fVec0[1];
			float fTemp23 = fTemp22 + fVec0[2];
			float fTemp24 = 0.33333334f * fTemp23;
			float fTemp25 = std::fabs(fTemp24);
			float fTemp26 = static_cast<float>((fTemp24 > 0.0f) - (fTemp24 < 0.0f));
			float fTemp27 = fTemp1 - fVec0[1];
			float fTemp28 = ClipEngine_faustpower2_f(fTemp27);
			int iTemp29 = fTemp28 <= 0.001f;
			float fTemp30 = std::fabs(fTemp1);
			int iTemp31 = fTemp30 < 0.33333334f;
			int iTemp32 = fTemp30 < 0.6666667f;
			float fTemp33 = ClipEngine_faustpower2_f(fTemp1);
			float fTemp34 = 0.5f * fTemp33;
			int iTemp35 = fTemp1 > 0.0f;
			float fTemp36 = static_cast<float>(((iTemp35) ? 1 : -1));
			float fTemp37 = ClipEngine_faustpower3_f(fTemp1);
			float fTemp38 = ((iTemp31) ? fTemp33 : ((iTemp32) ? 2.0f * fTemp33 + (0.037037037f - (0.33333334f * fTemp30 + ClipEngine_faustpower3_f(fTemp30))) : fTemp30 + -0.25925925f)) - fTemp16;
			float fTemp39 = fTemp1 + fTemp22;
			float fTemp40 = 0.33333334f * fTemp39;
			float fTemp41 = std::fabs(fTemp40);
			float fTemp42 = static_cast<float>((fTemp40 > 0.0f) - (fTemp40 < 0.0f));
			int iTemp43 = std::fabs(fTemp27) <= 0.001f;
			float fTemp44 = fTemp1 + fVec0[1];
			float fTemp45 = 0.5f * fTemp44;
			float fTemp46 = std::fabs(fTemp45);
			float fTemp47 = static_cast<float>((fTemp45 > 0.0f) - (fTemp45 < 0.0f));
			float fTemp48 = static_cast<float>(iTemp35 - (fTemp1 < 0.0f));
			int iTemp49 = (fVec0[2] <= 1.0f) & (fVec0[2] >= -1.0f);
			float fTemp50 = static_cast<float>(iTemp9 - (fVec0[2] < 0.0f));
			int iTemp51 = (fVec0[1] <= 1.0f) & (fVec0[1] >= -1.0f);
			float fTemp52 = static_cast<float>(iTemp18 - (fVec0[1] < 0.0f));
			float fTemp53 = ((iTemp51) ? fTemp17 : fVec0[1] * fTemp52 + -0.5f);
			float fTemp54 = ((iTemp51) ? 0.33333334f * fTemp20 : fTemp52 * (fTemp17 + -0.16666667f));
			int iTemp55 = (fTemp1 <= 1.0f) & (fTemp1 >= -1.0f);
			float fTemp56 = ((iTemp55) ? fTemp34 : fSlow4 * fTemp0 * fTemp48 + -0.5f) - fTemp53;
			output0[i0] = static_cast<FAUSTFLOAT>(fSlow3 * (fSlow7 * ((iSlow1) ? std::max<float>(-1.0f, std::min<float>(1.0f, fTemp1)) : ((iSlow2) ? ((iTemp43) ? std::max<float>(-1.0f, std::min<float>(1.0f, fTemp45)) : fTemp56 / fTemp27) : ((iTemp29) ? 0.5f * std::max<float>(-1.0f, std::min<float>(1.0f, fTemp40)) : (fSlow4 * fTemp0 * fTemp56 + fTemp54 - ((iTemp55) ? 0.33333334f * fTemp37 : fTemp48 * (fTemp34 + -0.16666667f))) / fTemp28) + ((iTemp2) ? 0.5f * std::max<float>(-1.0f, std::min<float>(1.0f, fTemp24)) : (fTemp54 + fVec0[2] * (((iTemp49) ? fTemp8 : fVec0[2] * fTemp50 + -0.5f) - fTemp53) - ((iTemp49) ? 0.33333334f * fTemp11 : fTemp50 * (fTemp8 + -0.16666667f))) / fTemp3))) + fSlow6 * ((iSlow1) ? ((iTemp31) ? fSlow5 * fTemp0 : ((iTemp32) ? 0.33333334f * fTemp48 * (3.0f - ClipEngine_faustpower2_f(2.0f - 3.0f * fTemp30)) : fTemp48)) : ((iSlow2) ? ((iTemp43) ? ((fTemp46 < 0.33333334f) ? fTemp44 : ((fTemp46 < 0.6666667f) ? 0.33333334f * fTemp47 * (3.0f - ClipEngine_faustpower2_f(2.0f - 3.0f * fTemp46)) : fTemp47)) : fTemp38 / fTemp27) : ((iTemp29) ? 0.5f * ((fTemp41 < 0.33333334f) ? 0.6666667f * fTemp39 : ((fTemp41 < 0.6666667f) ? 0.33333334f * fTemp42 * (3.0f - ClipEngine_faustpower2_f(2.0f - 3.0f * fTemp41)) : fTemp42)) : (fSlow4 * fTemp0 * fTemp38 + fTemp21 - ((iTemp31) ? 0.6666667f * fTemp37 : ((iTemp32) ? fTemp36 * (0.0030864198f - (0.16666667f * fTemp33 + 0.75f * ClipEngine_faustpower4_f(fTemp1))) + 1.3333334f * fTemp37 : fTemp36 * (fTemp34 + -0.046296295f)))) / fTemp28) + ((iTemp2) ? 0.5f * ((fTemp25 < 0.33333334f) ? 0.6666667f * fTemp23 : ((fTemp25 < 0.6666667f) ? 0.33333334f * fTemp26 * (3.0f - ClipEngine_faustpower2_f(2.0f - 3.0f * fTemp25)) : fTemp26)) : (fTemp21 + fVec0[2] * (((iTemp5) ? fTemp7 : ((iTemp6) ? 2.0f * fTemp7 + (0.037037037f - (0.33333334f * fTemp4 + ClipEngine_faustpower3_f(fTemp4))) : fTemp4 + -0.25925925f)) - fTemp16) - ((iTemp5) ? 0.6666667f * fTemp11 : ((iTemp6) ? fTemp10 * (0.0030864198f - (0.16666667f * fTemp7 + 0.75f * ClipEngine_faustpower4_f(fVec0[2]))) + 1.3333334f * fTemp11 : fTemp10 * (fTemp8 + -0.046296295f)))) / fTemp3)))));
			fVec0[2] = fVec0[1];
			fVec0[1] = fVec0[0];
		}
	}

};

#endif
