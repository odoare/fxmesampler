/*
  ==============================================================================

    SamplerComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Sampler.h"
#include "SampleGroupComponent.h"

class SamplerComponent : public juce::Component
{
public:
    SamplerComponent (Sampler& sampler, juce::AudioProcessorValueTreeState& apvts);
    ~SamplerComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Sampler& sampler;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Viewport viewport;
    std::unique_ptr<juce::Component> contentComp;
    std::vector<std::unique_ptr<SampleGroupComponent>> groupComponents;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerComponent)
};
