/*
  ==============================================================================

    EffectChainDynamicsComponent.h

  ==============================================================================
*/

#pragma once

#include "EffectChainComponent.h"
#include "EffectChainDynamics.h"
#include "EqualizerComponent.h"
#include "CompressorComponent.h"
#include "TubeComponent.h"

class EffectChainDynamicsComponent : public EffectChainComponent
{
public:
    EffectChainDynamicsComponent (EffectChainDynamics& chain, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void resized() override;

private:
    EffectChainDynamics& chain;
    EqualizerComponent eqComp;
    CompressorComponent compComp;
    TubeComponent tubeComp;
    
    juce::ComboBox orderBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> orderAtt;
};
