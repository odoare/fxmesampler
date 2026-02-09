/*
  ==============================================================================

    EffectChainReverbComponent.cpp

  ==============================================================================
*/

#include "EffectChainReverbComponent.h"

EffectChainReverbComponent::EffectChainReverbComponent (EffectChainReverb& c, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
    : chain (c),
      reverbComp (c.getReverb(), apvts, prefix)
{
    addAndMakeVisible (reverbComp);
}

void EffectChainReverbComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem;
    juce::FlexBox fbMain;
    fbMain.flexDirection = juce::FlexBox::Direction::column;
    
    fbMain.items.add(fi(reverbComp).withFlex(1.0f).withMargin(juce::FlexItem::Margin(3.f)));
    
    fbMain.performLayout(bounds);
}