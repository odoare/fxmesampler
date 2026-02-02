/*
  ==============================================================================

    EqualizerComponent.cpp

  ==============================================================================
*/

#include "EqualizerComponent.h"

EqualizerComponent::EqualizerComponent (Equalizer& eq) : equalizer (eq)
{
    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onButton.setToggleState (equalizer.isOn(), juce::dontSendNotification);
    onButton.addListener (this);

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
        s.addListener (this);
    };

    // Low Shelf
    setup (lsFreq, 20.0, 1000.0, 100.0);
    setup (lsGain, -15.0, 15.0, 0.0);

    // Band 1
    setup (b1Freq, 100.0, 5000.0, 500.0);
    setup (b1Q, 0.1, 10.0, 1.0);
    setup (b1Gain, -15.0, 15.0, 0.0);

    // Band 2
    setup (b2Freq, 500.0, 10000.0, 2000.0);
    setup (b2Q, 0.1, 10.0, 1.0);
    setup (b2Gain, -15.0, 15.0, 0.0);

    // High Shelf
    setup (hsFreq, 1000.0, 20000.0, 5000.0);
    setup (hsGain, -15.0, 15.0, 0.0);
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

void EqualizerComponent::sliderValueChanged (juce::Slider* slider)
{
    if (slider == &lsFreq || slider == &lsGain)
    {
        equalizer.setLowShelf ((float) lsFreq.getValue(), (float) lsGain.getValue());
    }
    else if (slider == &b1Freq || slider == &b1Q || slider == &b1Gain)
    {
        equalizer.setBand1 ((float) b1Freq.getValue(), (float) b1Q.getValue(), (float) b1Gain.getValue());
    }
    else if (slider == &b2Freq || slider == &b2Q || slider == &b2Gain)
    {
        equalizer.setBand2 ((float) b2Freq.getValue(), (float) b2Q.getValue(), (float) b2Gain.getValue());
    }
    else if (slider == &hsFreq || slider == &hsGain)
    {
        equalizer.setHighShelf ((float) hsFreq.getValue(), (float) hsGain.getValue());
    }
}

void EqualizerComponent::buttonClicked (juce::Button* button)
{
    if (button == &onButton)
        equalizer.setOn (onButton.getToggleState());
}