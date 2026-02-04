/*
  ==============================================================================

    TubeComponent.cpp

  ==============================================================================
*/

#include "TubeComponent.h"

TubeComponent::TubeComponent (Tube& t, juce::AudioProcessorValueTreeState& state, const juce::String& prefix)
    : tube (t), apvts (state)
{
    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, prefix + "_Tube_On", onButton);

    addAndMakeVisible (modelBox);
    modelBox.addItem ("Standard", 1);
    modelBox.addItem ("Dynamic", 2);
    modelAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, prefix + "_Tube_Model", modelBox);

    addAndMakeVisible (titleLabel);
    titleLabel.setText ("Tube Saturation", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (16.0f, juce::Font::bold));

    setupSlider (driveSlider, driveLabel, "Drive (dB)", 0.0, 40.0, 0.0);
    setupSlider (biasSlider, biasLabel, "Bias", 0.0, 0.5, 0.0);
    setupSlider (outSlider, outLabel, "Output (dB)", -20.0, 20.0, 0.0);

    driveAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, prefix + "_Tube_Drive", driveSlider);
    biasAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, prefix + "_Tube_Bias", biasSlider);
    outAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, prefix + "_Tube_Out", outSlider);
}

TubeComponent::~TubeComponent() {}

void TubeComponent::setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, double min, double max, double def)
{
    addAndMakeVisible (label);
    label.setText (text, juce::NotificationType::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 50, 20);
    slider.setRange (min, max);
    slider.setValue (def);

    slider.setLookAndFeel (&fxmeLookAndFeel);
}

void TubeComponent::paint (juce::Graphics& g)
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

void TubeComponent::resized()
{
    auto area = getLocalBounds().reduced (5);
    auto header = area.removeFromTop (25);
    onButton.setBounds (header.removeFromLeft (40));
    modelBox.setBounds (header.removeFromRight (80));
    titleLabel.setBounds (header);

    int w = area.getWidth() / 3;
    auto layout = [&](juce::Slider& s, juce::Label& l, int idx) { auto r = area.withX(area.getX() + idx*w).withWidth(w); l.setBounds(r.removeFromTop(20)); s.setBounds(r); };
    layout (driveSlider, driveLabel, 0);
    layout (biasSlider, biasLabel, 1);
    layout (outSlider, outLabel, 2);
}