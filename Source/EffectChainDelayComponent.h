/*
  ==============================================================================

    EffectChainDelayComponent.h

  ==============================================================================
*/

#pragma once

#include "EffectChainComponent.h"
#include "EffectChainDelay.h"
#include "StereoDelayComponent.h"

class EffectChainDelayComponent : public EffectChainComponent
{
public:
    EffectChainDelayComponent (EffectChainDelay& chain, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void resized() override;

private:
    EffectChainDelay& chain;
    StereoDelayComponent delayComp;
};