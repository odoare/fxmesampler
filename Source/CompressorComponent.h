/*
  ==============================================================================

    CompressorComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Compressor.h"
#include "VuMeterComponent.h"
#include "Knob.h"

class CompressorComponent : public juce::Component,
                            public juce::Timer
{
public:
    CompressorComponent (Compressor& compressorToControl, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~CompressorComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    Compressor& compressor;
    juce::AudioProcessorValueTreeState& apvts;

    juce::ToggleButton onButton;
    juce::Label titleLabel;
    VuMeterComponent grMeter;
    juce::Label preGainLabel, attackLabel, releaseLabel, threshLabel, ratioLabel, gainLabel;
    Knob preGainSlider, attackSlider, releaseSlider, threshSlider, ratioSlider, gainSlider;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ButtonAttachment> onAtt;
    std::unique_ptr<SliderAttachment> preGainAtt, attackAtt, releaseAtt, threshAtt, ratioAtt, gainAtt;

    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, double min, double max, double def);
    void setupBarSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, double min, double max, double def);
    void setSliderColours (juce::Slider& s, juce::Colour c);

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorComponent)
};