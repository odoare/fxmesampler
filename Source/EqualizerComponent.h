/*
  ==============================================================================

    EqualizerComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Equalizer.h"

class EqualizerComponent : public juce::Component
{
public:
    EqualizerComponent (Equalizer& equalizerToControl, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~EqualizerComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Equalizer& equalizer;
    juce::AudioProcessorValueTreeState& apvts;

    juce::ToggleButton onButton;
    juce::Label titleLabel;
    
    // LowShelf, Band1, Band2, HighShelf
    // LS: F, G. B1: F, Q, G. B2: F, Q, G. HS: F, G.
    juce::Slider lsFreq, lsGain;
    juce::Slider b1Freq, b1Q, b1Gain;
    juce::Slider b2Freq, b2Q, b2Gain;
    juce::Slider hsFreq, hsGain;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ButtonAttachment> onAtt;
    std::unique_ptr<SliderAttachment> lsFreqAtt, lsGainAtt;
    std::unique_ptr<SliderAttachment> b1FreqAtt, b1QAtt, b1GainAtt;
    std::unique_ptr<SliderAttachment> b2FreqAtt, b2QAtt, b2GainAtt;
    std::unique_ptr<SliderAttachment> hsFreqAtt, hsGainAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqualizerComponent)
};