/*
  ==============================================================================

    CompressorComponent.cpp

  ==============================================================================
*/

#include "CompressorComponent.h"

CompressorComponent::CompressorComponent (Compressor& comp) : compressor (comp)
{
    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onButton.setToggleState (compressor.isOn(), juce::dontSendNotification);
    onButton.addListener (this);

    addAndMakeVisible (titleLabel);
    titleLabel.setText ("Compressor", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (16.0f, juce::Font::bold));

    setupSlider (attackSlider, attackLabel, "Attack (ms)", 0.1, 100.0, 10.0);
    setupSlider (releaseSlider, releaseLabel, "Release (ms)", 10.0, 1000.0, 100.0);
    setupSlider (threshSlider, threshLabel, "Thresh (dB)", -60.0, 0.0, 0.0);
    setupSlider (ratioSlider, ratioLabel, "Ratio", 1.0, 20.0, 1.0);
    setupSlider (gainSlider, gainLabel, "Gain (dB)", 0.0, 24.0, 0.0);
}

CompressorComponent::~CompressorComponent()
{
}

void CompressorComponent::setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, double min, double max, double def)
{
    addAndMakeVisible (label);
    label.setText (text, juce::NotificationType::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    slider.setRange (min, max);
    slider.setValue (def);
    slider.addListener (this);
}

void CompressorComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey.darker (0.2f));
    g.setColour (juce::Colours::white);
    g.drawRect (getLocalBounds(), 1);
}

void CompressorComponent::resized()
{
    auto area = getLocalBounds().reduced (5);
    auto header = area.removeFromTop (25);
    onButton.setBounds (header.removeFromLeft (40));
    titleLabel.setBounds (header);

    int w = area.getWidth() / 5;
    
    auto layout = [&](juce::Slider& s, juce::Label& l, int index)
    {
        auto r = area.withX (area.getX() + index * w).withWidth (w);
        l.setBounds (r.removeFromTop (20));
        s.setBounds (r);
    };

    layout (attackSlider, attackLabel, 0);
    layout (releaseSlider, releaseLabel, 1);
    layout (threshSlider, threshLabel, 2);
    layout (ratioSlider, ratioLabel, 3);
    layout (gainSlider, gainLabel, 4);
}

void CompressorComponent::sliderValueChanged (juce::Slider* slider)
{
    if (slider == &attackSlider)
        compressor.setAttack ((float) slider->getValue());
    else if (slider == &releaseSlider)
        compressor.setRelease ((float) slider->getValue());
    else if (slider == &threshSlider)
        compressor.setThreshold ((float) slider->getValue());
    else if (slider == &ratioSlider)
        compressor.setRatio ((float) slider->getValue());
    else if (slider == &gainSlider)
        compressor.setPostGain ((float) slider->getValue());
}

void CompressorComponent::buttonClicked (juce::Button* button)
{
    if (button == &onButton)
        compressor.setOn (onButton.getToggleState());
}