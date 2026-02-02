/*
  ==============================================================================

    MixerComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Mixer.h"
#include "CompressorComponent.h"
#include "EqualizerComponent.h"

class MixerComponent : public juce::Component
{
public:
    MixerComponent (Mixer& mixer);
    ~MixerComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Mixer& mixer;
    juce::TabbedComponent tabs;

    // Tab 1: Levels
    class LevelsComponent : public juce::Component, public juce::Slider::Listener
    {
    public:
        LevelsComponent (Mixer& m);
        void resized() override;
        void sliderValueChanged (juce::Slider* slider) override;
    private:
        Mixer& mixer;
        juce::Label ambLabel, ambLvlLabel, kickLabel, snareLabel, hhLabel;
        juce::Slider azSlider, elSlider, wSlider, ambLvlSlider, ambLvlSlider2;
        juce::Slider kickLvl, kickPan, snareLvl, snarePan, hhLvl, hhPan;
    };
    
    LevelsComponent levelsComp;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerComponent)
};