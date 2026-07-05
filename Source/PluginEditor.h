#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <memory>
#include <optional>
#include <vector>
#include "PluginProcessor.h"

//==============================================================================
// JUCE 8 WebView editor. All parameters are bound to the HTML UI via relays;
// live meter levels are pushed to the UI on a timer.
//
// CRITICAL member declaration order (crash-on-unload guard):
//   relays -> WebBrowserComponent -> parameter attachments
// (destruction is the reverse of declaration).
class DetailForgeEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit DetailForgeEditor (DetailForgeProcessor&);
    ~DetailForgeEditor() override;

    void resized() override;
    void timerCallback() override;

private:
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    DetailForgeProcessor& processorRef;

    // Transfer-curve push state (plain data — no bearing on the crash-order rule below).
    std::vector<float> transferY;
    juce::String       lastTransferKey;

    // 1) relays FIRST
    std::vector<std::unique_ptr<juce::WebSliderRelay>>      sliderRelays;
    std::vector<std::unique_ptr<juce::WebComboBoxRelay>>    comboRelays;
    std::unique_ptr<juce::WebToggleButtonRelay>            bypassRelay;

    // 2) the browser SECOND
    std::unique_ptr<juce::WebBrowserComponent>            webView;

    // 3) attachments LAST
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>    sliderAtts;
    std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>>  comboAtts;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment>          bypassAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DetailForgeEditor)
};
