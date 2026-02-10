/*
  ==============================================================================

    EffectChainReverbComponent.cpp

  ==============================================================================
*/

#include "EffectChainReverbComponent.h"

EffectChainReverbComponent::EffectChainReverbComponent (EffectChainReverb& c, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
    : chain (c),
      reverbComp (c.getReverb(), apvts, prefix),
      eqComp (c.getEQ(), apvts, prefix),
      delayComp (c.getDelay(), apvts, prefix)
{
    addAndMakeVisible (reverbComp);
    addAndMakeVisible (delayComp);
    addAndMakeVisible (eqComp);
}

void EffectChainReverbComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem;
    juce::FlexBox fbMain, fbRevDel;
    fbMain.flexDirection = juce::FlexBox::Direction::row;
    fbRevDel.flexDirection = juce::FlexBox::Direction::column;
    
    fbRevDel.items.add(fi(reverbComp).withFlex(1.0f).withMargin(juce::FlexItem::Margin(3.f)));
    fbRevDel.items.add(fi(delayComp).withFlex(0.9f).withMargin(juce::FlexItem::Margin(3.f)));

    fbMain.items.add(fi(fbRevDel).withFlex(1.0f).withMargin(juce::FlexItem::Margin(3.f)));
    fbMain.items.add(fi(eqComp).withFlex(1.0f).withMargin(juce::FlexItem::Margin(3.f)));
    
    fbMain.performLayout(bounds);
}