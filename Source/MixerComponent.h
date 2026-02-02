/*
  ==============================================================================

    MixerComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Mixer.h"
#include "CompressorComponent.h"
#include "VuMeterComponent.h"
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

    // Dynamic Levels Component
    class LevelsComponent : public juce::Component, public juce::Slider::Listener, public juce::Timer
    {
    public:
        LevelsComponent (Mixer& m);
        void resized() override;
        void sliderValueChanged (juce::Slider* slider) override;
        void timerCallback() override;
        
    private:
        Mixer& mixer;
        
        struct StripControls {
            std::unique_ptr<juce::Label> label;
            std::vector<std::unique_ptr<juce::Slider>> sliders;
            std::vector<std::unique_ptr<VuMeterComponent>> meters;
        };
        std::vector<StripControls> stripControls;
        
        juce::Label vuLabel;
    };
    
    LevelsComponent levelsComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerComponent)
};