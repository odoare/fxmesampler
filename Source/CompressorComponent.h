/*
  ==============================================================================

    CompressorComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Compressor.h"

class CompressorComponent : public juce::Component,
                            public juce::Slider::Listener,
                            public juce::Button::Listener
{
public:
    CompressorComponent (Compressor& compressorToControl);
    ~CompressorComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged (juce::Slider* slider) override;
    void buttonClicked (juce::Button* button) override;

private:
    Compressor& compressor;

    juce::ToggleButton onButton;
    juce::Label titleLabel;
    juce::Label attackLabel, releaseLabel, threshLabel, ratioLabel, gainLabel;
    juce::Slider attackSlider, releaseSlider, threshSlider, ratioSlider, gainSlider;

    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, double min, double max, double def);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorComponent)
};