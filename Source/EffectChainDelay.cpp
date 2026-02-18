/*
  ==============================================================================

    EffectChainDelay.cpp

  ==============================================================================
*/

#include "EffectChainDelay.h"

EffectChainDelay::EffectChainDelay()
{
}

void EffectChainDelay::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    delay.prepare (sampleRate, samplesPerBlock);
    juce::ignoreUnused(numChannels);
}

void EffectChainDelay::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    delay.assignParameters (apvts, prefix);
}

void EffectChainDelay::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix)
{
    StereoDelay::addParameters (params, prefix);
}

void EffectChainDelay::process (juce::AudioBuffer<float>& buffer)
{
    delay.process(buffer);
}

void EffectChainDelay::setBPM(double bpm)
{
    delay.setBPM(bpm);
}