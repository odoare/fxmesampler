/*
  ==============================================================================

    PluginEditor.cpp

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

SimpleSamplerAudioProcessorEditor::SimpleSamplerAudioProcessorEditor (SimpleSamplerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), mixerComponent (p.getMixer(), p.getAPVTS())
{
    addAndMakeVisible (mixerComponent);
    setSize (600, 500);
}

SimpleSamplerAudioProcessorEditor::~SimpleSamplerAudioProcessorEditor()
{
}

void SimpleSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void SimpleSamplerAudioProcessorEditor::resized()
{
    mixerComponent.setBounds (getLocalBounds());
}