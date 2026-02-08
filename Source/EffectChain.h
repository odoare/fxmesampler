/*
  ==============================================================================

    EffectChain.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class EffectChain
{
public:
    virtual ~EffectChain() = default;

    virtual void prepare (double sampleRate, int samplesPerBlock, int numChannels) = 0;
    virtual void process (juce::AudioBuffer<float>& buffer) = 0;
    virtual void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix) = 0;
};
