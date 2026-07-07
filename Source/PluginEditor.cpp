#include "PluginEditor.h"
#include "BinaryData.h"
#include <array>

namespace
{
    // Continuous params -> WebSliderRelay. Choice params -> WebComboBoxRelay.
    const juce::StringArray kSliderIds {
        "in_gain", "out_gain",
        "sat_drive", "sat_bias", "sat_mix",
        "clip_drive", "clip_knee", "clip_ceiling", "clip_detail", "clip_detail_freq",
        "lim_threshold", "lim_ceiling", "lim_release", "lim_lookahead"
    };
    const juce::StringArray kComboIds { "sat_voicing", "oversampling", "adaa_order" };
    const juce::StringArray kToggleIds { "sat_on", "clip_on", "lim_on" };   // section enables

    juce::WebBrowserComponent::Resource makeResource (const char* data, int size, juce::String mime)
    {
        std::vector<std::byte> bytes ((size_t) size);
        std::memcpy (bytes.data(), data, (size_t) size);
        return { std::move (bytes), std::move (mime) };
    }
}

DetailForgeEditor::DetailForgeEditor (DetailForgeProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // --- relays (must exist before the browser that references them) ---
    for (auto& id : kSliderIds)
        sliderRelays.push_back (std::make_unique<juce::WebSliderRelay> (id));
    for (auto& id : kComboIds)
        comboRelays.push_back (std::make_unique<juce::WebComboBoxRelay> (id));
    bypassRelay = std::make_unique<juce::WebToggleButtonRelay> ("bypass");
    for (auto& id : kToggleIds)
        toggleRelays.push_back (std::make_unique<juce::WebToggleButtonRelay> (id));

    // --- browser options: common parts, then a platform-specific backend ---
    // The UI is one HTML/JS codebase; only the native WebView backend differs per OS.
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withResourceProvider ([this] (const auto& url) { return getResource (url); });

   #if JUCE_WINDOWS
    // Windows: the WebView2 (Chromium) backend + a persistent per-plugin user-data folder.
    // The user-data folder is required — without it WebView2 misbehaves inside a plugin host.
    options = options
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (juce::File::getSpecialLocation (juce::File::SpecialLocationType::userApplicationDataDirectory)
                                     .getChildFile ("Fayella").getChildFile ("DetailForge").getChildFile ("WebView2")));
   #endif
    // macOS uses the default WKWebView backend — no runtime dependency, no WebView2-only options.

    for (auto& r : sliderRelays) options = options.withOptionsFrom (*r);
    for (auto& r : comboRelays)  options = options.withOptionsFrom (*r);
    options = options.withOptionsFrom (*bypassRelay);
    for (auto& r : toggleRelays) options = options.withOptionsFrom (*r);

    webView = std::make_unique<juce::WebBrowserComponent> (options);
    addAndMakeVisible (*webView);

    // --- attachments (last: destroyed first) ---
    for (int i = 0; i < kSliderIds.size(); ++i)
        if (auto* param = processorRef.apvts.getParameter (kSliderIds[i]))
            sliderAtts.push_back (std::make_unique<juce::WebSliderParameterAttachment> (
                *param, *sliderRelays[(size_t) i], nullptr));

    for (int i = 0; i < kComboIds.size(); ++i)
        if (auto* param = processorRef.apvts.getParameter (kComboIds[i]))
            comboAtts.push_back (std::make_unique<juce::WebComboBoxParameterAttachment> (
                *param, *comboRelays[(size_t) i], nullptr));

    if (auto* param = processorRef.apvts.getParameter ("bypass"))
        bypassAtt = std::make_unique<juce::WebToggleButtonParameterAttachment> (
            *param, *bypassRelay, nullptr);

    for (int i = 0; i < kToggleIds.size(); ++i)
        if (auto* param = processorRef.apvts.getParameter (kToggleIds[i]))
            toggleAtts.push_back (std::make_unique<juce::WebToggleButtonParameterAttachment> (
                *param, *toggleRelays[(size_t) i], nullptr));

    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // Free resize; the HTML scales its fixed-size layout to fill the window (zoom).
    setResizable (true, true);
    setResizeLimits (760, 500, 2400, 1560);
    setSize (juce::jlimit (760, 2400, processorRef.editorW),
             juce::jlimit (500, 1560, processorRef.editorH));   // restore the saved size
    startTimerHz (30);
}

DetailForgeEditor::~DetailForgeEditor() { stopTimer(); }

void DetailForgeEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
    processorRef.editorW = getWidth();    // remember the size for reopen / state save
    processorRef.editorH = getHeight();
}

void DetailForgeEditor::timerCallback()
{
    if (webView == nullptr)
        return;

    // Meters.
    juce::DynamicObject::Ptr o = new juce::DynamicObject();
    o->setProperty ("in",  (double) processorRef.inLevelDb.load());
    o->setProperty ("out", (double) processorRef.outLevelDb.load());
    o->setProperty ("gr",  (double) processorRef.grDb.load());
    webView->emitEventIfBrowserIsVisible ("meters", juce::var (o.get()));

    // Live scope waveform: pack the last N in/out/clip samples and ship as base64.
    // 4096 (~85 ms @48k) so low notes (a B0 saw's ~32 ms period) fit and can be sync-locked.
    constexpr int N = 4096;
    std::array<float, 3 * N> packed;
    processorRef.readScope (packed.data(), packed.data() + N, packed.data() + 2 * N, N);
    auto b64 = juce::Base64::toBase64 (packed.data(), sizeof (float) * packed.size());

    juce::DynamicObject::Ptr w = new juce::DynamicObject();
    w->setProperty ("n",    N);
    w->setProperty ("sr",   processorRef.currentSampleRate);
    w->setProperty ("data", b64);
    webView->emitEventIfBrowserIsVisible ("wave", juce::var (w.get()));

    // Transfer curve: recompute + push ONLY when a curve-shaping param changed (throttled to the
    // timer rate). Cheap probe (~528 samples through the offline engine), so 30 Hz worst case.
    {
        auto rv = [this] (const char* id) { auto* p = processorRef.apvts.getRawParameterValue (id); return p ? p->load() : 0.0f; };
        juce::String key;
        for (auto* id : { "sat_voicing","sat_drive","sat_bias","sat_mix","clip_drive","clip_knee","clip_ceiling","adaa_order" })
            key << juce::String (rv (id), 3) << ",";
        if (key != lastTransferKey)
        {
            lastTransferKey = key;
            processorRef.computeTransferCurve (transferY, 512);
            auto tb64 = juce::Base64::toBase64 (transferY.data(), sizeof (float) * transferY.size());
            juce::DynamicObject::Ptr t = new juce::DynamicObject();
            t->setProperty ("n",    (int) transferY.size());
            t->setProperty ("data", tb64);
            webView->emitEventIfBrowserIsVisible ("transfer", juce::var (t.get()));
        }
    }
}

std::optional<juce::WebBrowserComponent::Resource> DetailForgeEditor::getResource (const juce::String& url)
{
    const auto path = url.endsWithChar ('/') ? juce::String ("/index.html") : url;

    if (path == "/index.html" || path == "/")
        return makeResource (BinaryData::index_html, BinaryData::index_htmlSize, "text/html");

    // JUCE frontend module (index.js) and its dependency (check_native_interop.js).
    if (path.endsWith ("check_native_interop.js"))
        return makeResource (BinaryData::juce_check_native_interop_js,
                             BinaryData::juce_check_native_interop_jsSize, "text/javascript");

    if (path.endsWith ("index.js"))
        return makeResource (BinaryData::juce_index_js, BinaryData::juce_index_jsSize, "text/javascript");

    return std::nullopt;
}
