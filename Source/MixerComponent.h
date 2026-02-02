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
    MixerComponent (Mixer& mixer, juce::AudioProcessorValueTreeState& apvts);
    ~MixerComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Mixer& mixer;
    juce::AudioProcessorValueTreeState& apvts;
    juce::TabbedComponent tabs;

    // Dynamic Levels Component
    class LevelsComponent : public juce::Component, public juce::Timer
    {
    public:
        LevelsComponent (Mixer& m, juce::AudioProcessorValueTreeState& apvts);
        void resized() override;
        void timerCallback() override;
        
    private:
        Mixer& mixer;
        juce::AudioProcessorValueTreeState& apvts;
        
        struct StripControls {
            std::unique_ptr<juce::Label> label;
            std::vector<std::unique_ptr<juce::Slider>> sliders;
            std::vector<std::unique_ptr<VuMeterComponent>> meters;
            std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
        };
        std::vector<StripControls> stripControls;
        
        juce::Label vuLabel;
    };
    
    LevelsComponent levelsComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerComponent)
};