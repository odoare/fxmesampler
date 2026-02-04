/*
  ==============================================================================

    TubeComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Tube.h"

class TubeComponent : public juce::Component
{
public:
    TubeComponent (Tube& tube, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~TubeComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Tube& tube;
    juce::AudioProcessorValueTreeState& apvts;

    juce::ToggleButton onButton;
    juce::ComboBox modelBox;
    juce::Label titleLabel;
    juce::Label driveLabel, biasLabel, outLabel;
    juce::Slider driveSlider, biasSlider, outSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modelAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAtt, biasAtt, outAtt;

    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, double min, double max, double def);

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TubeComponent)
};