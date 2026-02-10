/*
  ==============================================================================

    EffectChainDelayComponent.cpp

  ==============================================================================
*/

#include "EffectChainDelayComponent.h"

EffectChainDelayComponent::EffectChainDelayComponent (EffectChainDelay& c, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
    : chain (c),
      delayComp (c.getDelay(), apvts, prefix)
{
    addAndMakeVisible (delayComp);
}

void EffectChainDelayComponent::resized()
{
    delayComp.setBounds(getLocalBounds());
}