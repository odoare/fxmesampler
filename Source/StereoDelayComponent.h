/*
  ==============================================================================

    StereoDelayComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "StereoDelay.h"
#include "ConvolReverbComponent.h" // For FxmeLookAndFeel
#include "Knob.h"

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
    Knob delayLSlider, delayRSlider;
    juce::Label delayLLabel, delayRLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayLAtt, delayRAtt;

    Knob fdbkLSlider, fdbkRSlider;
    juce::Label fdbkLLabel, fdbkRLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fdbkLAtt, fdbkRAtt;

    Knob crossFdbkSlider;
    juce::Label crossFdbkLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> crossFdbkAtt;

    Knob cutoffSlider, qSlider;
    juce::Label cutoffLabel, qLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAtt, qAtt;

    Knob outGainSlider;
    juce::Label outGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outGainAtt;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& text);
    void setSliderColours(juce::Slider& s, juce::Colour c);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StereoDelayComponent)
};