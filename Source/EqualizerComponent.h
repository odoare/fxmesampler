/*
  ==============================================================================

    EqualizerComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Equalizer.h"

class EqualizerComponent : public juce::Component,
                           public juce::Slider::Listener,
                           public juce::Button::Listener
{
public:
    EqualizerComponent (Equalizer& equalizerToControl);
    ~EqualizerComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged (juce::Slider* slider) override;
    void buttonClicked (juce::Button* button) override;

private:
    Equalizer& equalizer;

    juce::ToggleButton onButton;
    juce::Label titleLabel;
    
    // LowShelf, Band1, Band2, HighShelf
    // LS: F, G. B1: F, Q, G. B2: F, Q, G. HS: F, G.
    juce::Slider lsFreq, lsGain;
    juce::Slider b1Freq, b1Q, b1Gain;
    juce::Slider b2Freq, b2Q, b2Gain;
    juce::Slider hsFreq, hsGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqualizerComponent)
};