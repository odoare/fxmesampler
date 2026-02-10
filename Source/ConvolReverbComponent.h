/*
  ==============================================================================

    ConvolReverbComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ConvolReverb.h"

class ConvolReverbComponent : public juce::Component, public juce::Timer
{
public:
    ConvolReverbComponent (ConvolReverb& reverbToControl, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~ConvolReverbComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    ConvolReverb& reverb;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label titleLabel;

    juce::ComboBox irBox;
    juce::Label irLabel;
    juce::Slider lengthSlider;
    juce::Label lengthLabel;
    juce::Slider startOffsetSlider;
    juce::Label startOffsetLabel;
    juce::ComboBox shapeBox;
    juce::Label shapeLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> irAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lengthAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> startOffsetAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAtt;

    juce::Path irPlotPathL, irPlotPathR, irMaxPathL, irMaxPathR;
    void updateGraph();    
    std::atomic<bool> graphNeedsUpdate { false };
    
    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, double min, double max, double def);
    void setSliderColours (juce::Slider& s, juce::Colour c);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConvolReverbComponent)
};