/*
  ==============================================================================

    SamplerComponent.cpp

  ==============================================================================
*/

#include "SamplerComponent.h"

SamplerComponent::SamplerComponent (Sampler& s, juce::AudioProcessorValueTreeState& state)
    : sampler (s), apvts (state)
{
    contentComp = std::make_unique<juce::Component>();
    
    for (auto& group : sampler.getSampleGroups())
    {
        auto comp = std::make_unique<SampleGroupComponent> (*group, apvts);
        contentComp->addAndMakeVisible (*comp);
        groupComponents.push_back (std::move (comp));
    }

    addAndMakeVisible (viewport);
    viewport.setViewedComponent (contentComp.get(), false);
    viewport.setScrollBarsShown (true, false);
}

SamplerComponent::~SamplerComponent()
{
}

void SamplerComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void SamplerComponent::resized()
{
    viewport.setBounds (getLocalBounds());

    juce::FlexBox fbMain;

    fbMain.flexDirection = juce::FlexBox::Direction::column;
    using fi = juce::FlexItem;

    for (auto& comp : groupComponents)
    {
        fbMain.items.add(fi(*comp).withFlex(0.0f).withHeight(120.0f));
    }

    contentComp->setBounds(0, 0, viewport.getMaximumVisibleWidth(), 120 * (int)groupComponents.size());
    fbMain.performLayout(contentComp->getLocalBounds().toFloat());
}
