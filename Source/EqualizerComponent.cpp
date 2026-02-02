/*
  ==============================================================================

    EqualizerComponent.cpp

  ==============================================================================
*/

#include "EqualizerComponent.h"

EqualizerComponent::EqualizerComponent (Equalizer& eq, juce::AudioProcessorValueTreeState& state, const juce::String& prefix)
    : equalizer (eq), apvts (state)
{
    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onAtt = std::make_unique<ButtonAttachment> (apvts, prefix + "_EQ_On", onButton);

    addAndMakeVisible (titleLabel);
    titleLabel.setText ("Equalizer (LS / Peak / Peak / HS)", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (16.0f, juce::Font::bold));

    auto setup = [&](juce::Slider& s, double min, double max, double def) {
        addAndMakeVisible (s);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 40, 15);
        s.setRange (min, max);
        s.setValue (def);
    };

    // Low Shelf
    setup (lsFreq, 20.0, 1000.0, 100.0);
    setup (lsGain, -15.0, 15.0, 0.0);
    lsFreqAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_LS_Freq", lsFreq);
    lsGainAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_LS_Gain", lsGain);

    // Band 1
    setup (b1Freq, 100.0, 5000.0, 500.0);
    setup (b1Q, 0.1, 10.0, 1.0);
    setup (b1Gain, -15.0, 15.0, 0.0);
    b1FreqAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_B1_Freq", b1Freq);
    b1QAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_B1_Q", b1Q);
    b1GainAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_B1_Gain", b1Gain);

    // Band 2
    setup (b2Freq, 500.0, 10000.0, 2000.0);
    setup (b2Q, 0.1, 10.0, 1.0);
    setup (b2Gain, -15.0, 15.0, 0.0);
    b2FreqAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_B2_Freq", b2Freq);
    b2QAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_B2_Q", b2Q);
    b2GainAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_B2_Gain", b2Gain);

    // High Shelf
    setup (hsFreq, 1000.0, 20000.0, 5000.0);
    setup (hsGain, -15.0, 15.0, 0.0);
    hsFreqAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_HS_Freq", hsFreq);
    hsGainAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_HS_Gain", hsGain);
}

EqualizerComponent::~EqualizerComponent()
{
}

void EqualizerComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey.darker (0.2f));
    g.setColour (juce::Colours::white);
    g.drawRect (getLocalBounds(), 1);
}

void EqualizerComponent::resized()
{
    auto area = getLocalBounds().reduced (5);
    auto header = area.removeFromTop (25);
    onButton.setBounds (header.removeFromLeft (40));
    titleLabel.setBounds (header);

    int w = area.getWidth() / 4;
    
    auto layoutBand = [&](int idx, juce::Slider* s1, juce::Slider* s2, juce::Slider* s3)
    {
        auto r = area.withX (area.getX() + idx * w).withWidth (w);
        int h = r.getHeight() / 3;
        if (s1) s1->setBounds (r.removeFromTop (h));
        if (s2) s2->setBounds (r.removeFromTop (h));
        if (s3) s3->setBounds (r.removeFromTop (h));
    };

    layoutBand (0, &lsFreq, &lsGain, nullptr);
    layoutBand (1, &b1Freq, &b1Q, &b1Gain);
    layoutBand (2, &b2Freq, &b2Q, &b2Gain);
    layoutBand (3, &hsFreq, &hsGain, nullptr);
}