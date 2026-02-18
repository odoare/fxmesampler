/*
  ==============================================================================

    StereoDelayComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "StereoDelay.h"
#include "ConvolReverbComponent.h" // For FxmeLookAndFeel
#include "FxmeSlider.h"

class StereoDelayComponent : public juce::Component, public juce::Timer
{
public:
    StereoDelayComponent(StereoDelay& delayToControl, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~StereoDelayComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    StereoDelay& delay;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label titleLabel;
    juce::ToggleButton onButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;

    juce::Label bpmLabel;
    FxmeSlider delayLSlider, delayRSlider;
    juce::Label delayLLabel, delayRLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayLAtt, delayRAtt;

    FxmeSlider fdbkLSlider, fdbkRSlider;
    juce::Label fdbkLLabel, fdbkRLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fdbkLAtt, fdbkRAtt;

    FxmeSlider crossFdbkSlider;
    juce::Label crossFdbkLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> crossFdbkAtt;

    FxmeSlider cutoffSlider, qSlider;
    juce::Label cutoffLabel, qLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAtt, qAtt;

    FxmeSlider dryGainSlider, wetGainSlider;
    juce::Label dryGainLabel, wetGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryGainAtt, wetGainAtt;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& text);
    void setupBarSlider(juce::Slider& slider, juce::Label& label, const juce::String& text);
    void setSliderColours(juce::Slider& s, juce::Colour c);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StereoDelayComponent)
};