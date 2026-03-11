/*
  ==============================================================================

    PluginEditor.cpp

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeSamplerAudioProcessorEditor::FxmeSamplerAudioProcessorEditor (FxmeSamplerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), 
      mixerComponent (p.getMixer(), p.getSampler(), p.getAPVTS())
{
    addAndMakeVisible (mixerComponent);
    int n = p.getMixer().getStrips().size();
    setSize (juce::jmax<int>(n*120,1024), 640);
}

FxmeSamplerAudioProcessorEditor::~FxmeSamplerAudioProcessorEditor()
{
}

void FxmeSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto diagonale = (getLocalBounds().getTopLeft() - getLocalBounds().getBottomRight()).toFloat();
    auto length = diagonale.getDistanceFromOrigin();
    auto perpendicular = diagonale.rotatedAboutOrigin (juce::degreesToRadians (270.0f)) / length;
    auto height = float (getWidth() * getHeight()) / length;
    auto bluegreengrey = juce::Colour::fromFloatRGBA (0.15f, 0.15f, 0.25f, 1.0f);
    juce::ColourGradient grad (bluegreengrey.darker().darker().darker(), perpendicular * height,
                           bluegreengrey, perpendicular * -height, false);
    g.setGradientFill(grad);
    g.fillAll();
}

void FxmeSamplerAudioProcessorEditor::resized()
{
    mixerComponent.setBounds (getLocalBounds());
}