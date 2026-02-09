/*
  ==============================================================================

    EffectChainReverb.cpp

  ==============================================================================
*/

#include "EffectChainReverb.h"

EffectChainReverb::EffectChainReverb()
{
}

void EffectChainReverb::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    reverb.prepare (sampleRate, samplesPerBlock);
    juce::ignoreUnused (numChannels);
}

void EffectChainReverb::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    reverb.assignParameters (apvts, prefix);
}

void EffectChainReverb::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix)
{
    reverb.addParameters (params, prefix);
}

void EffectChainReverb::process (juce::AudioBuffer<float>& buffer)
{
    // ConvolReverb's process method now handles parameter checking internally
    // based on its assigned parameters.
    reverb.process (buffer);
}