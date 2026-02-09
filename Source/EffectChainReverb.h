/*
  ==============================================================================

    EffectChainReverb.h

  ==============================================================================
*/

#pragma once

#include "EffectChain.h"
#include "ConvolReverb.h"

class EffectChainReverb : public EffectChain
{
public:
    EffectChainReverb();
    
    void prepare (double sampleRate, int samplesPerBlock, int numChannels) override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix) override;
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix) override;

    ConvolReverb& getReverb() { return reverb; }

private:
    ConvolReverb reverb;
};