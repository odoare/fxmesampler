/*
  ==============================================================================

    CompressorComponent.cpp

  ==============================================================================
*/

#include "CompressorComponent.h"

CompressorComponent::CompressorComponent (Compressor& comp, juce::AudioProcessorValueTreeState& state, const juce::String& prefix)
    : compressor (comp), apvts (state)
{
    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onAtt = std::make_unique<ButtonAttachment> (apvts, prefix + "_Comp_On", onButton);

    addAndMakeVisible (titleLabel);
    titleLabel.setText ("Compressor", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (16.0f, juce::Font::bold));

    setupSlider (attackSlider, attackLabel, "Attack (ms)", 0.1, 100.0, 10.0);
    setupSlider (releaseSlider, releaseLabel, "Release (ms)", 10.0, 1000.0, 100.0);
    setupSlider (threshSlider, threshLabel, "Thresh (dB)", -60.0, 0.0, 0.0);
    setupSlider (ratioSlider, ratioLabel, "Ratio", 1.0, 20.0, 1.0);
    setupSlider (gainSlider, gainLabel, "Gain (dB)", 0.0, 24.0, 0.0);

    attackAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_Comp_Attack", attackSlider);
    releaseAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_Comp_Release", releaseSlider);
    threshAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_Comp_Thresh", threshSlider);
    ratioAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_Comp_Ratio", ratioSlider);
    gainAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_Comp_Gain", gainSlider);

    addAndMakeVisible (grMeter);
    grMeter.setMeterColor (juce::Colours::red);
    grMeter.setRange (-30.0f, 0.0f);
    grMeter.setZeroLevel (0.0f);
    startTimerHz (24);
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
}

void CompressorComponent::paint (juce::Graphics& g)
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

void CompressorComponent::resized()
{
    auto area = getLocalBounds().reduced (5);
    auto header = area.removeFromTop (25);
    onButton.setBounds (header.removeFromLeft (40));
    titleLabel.setBounds (header);

    auto meterArea = area.removeFromRight (20);
    grMeter.setBounds (meterArea);
    
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

void CompressorComponent::timerCallback()
{
    grMeter.setValue (compressor.getGrMeter().getPeak());
}