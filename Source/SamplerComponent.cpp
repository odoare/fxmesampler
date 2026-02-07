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
    
    int itemHeight = 150;
    int itemWidth = 280;
    int margin = 10;
    
    int x = margin;
    int y = margin;
    
    int width = getWidth() - viewport.getScrollBarThickness();
    if (width <= 0) width = getWidth();

    for (auto& comp : groupComponents)
    {
        if (x + itemWidth > width && x > margin)
        {
            x = margin;
            y += itemHeight + margin;
        }
        
        comp->setBounds (x, y, itemWidth, itemHeight);
        x += itemWidth + margin;
    }
    
    // Ensure the content component is large enough to scroll
    contentComp->setBounds (0, 0, width, y + itemHeight + margin);
}
