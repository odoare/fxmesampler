/*
  ==============================================================================

    EffectChainReverbComponent.h

  ==============================================================================
*/

#pragma once

#include "EffectChainComponent.h"
#include "EffectChainReverb.h"
#include "ConvolReverbComponent.h"
#include "EqualizerComponent.h"
#include "StereoDelayComponent.h"

class EffectChainReverbComponent : public EffectChainComponent
{
public:
    EffectChainReverbComponent (EffectChainReverb& chain, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void resized() override;

private:
    EffectChainReverb& chain;
    ConvolReverbComponent reverbComp;
    EqualizerComponent eqComp;
    StereoDelayComponent delayComp;
};