/*
  ==============================================================================

    SampleGroupComponent.cpp

  ==============================================================================
*/

#include "SampleGroupComponent.h"

void SampleGroupComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

SampleGroupComponent::SampleGroupComponent (SampleGroup& g, juce::AudioProcessorValueTreeState& state)
    : group (g), apvts (state)
{
    addAndMakeVisible (nameLabel);
    nameLabel.setText (group.getName(), juce::NotificationType::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setFont (juce::Font (16.0f, juce::Font::bold));

    addAndMakeVisible (oneShotButton);
    oneShotButton.setButtonText ("One Shot");
    oneShotButton.setLookAndFeel (&fxmeLookAndFeel);
    oneShotButton.setColour (juce::ToggleButton::tickColourId, juce::Colours::cyan);
    
    // Assuming parameter naming convention: GroupName_Parameter
    juce::String prefix = group.getName() + "_";
    
    oneShotAtt = std::make_unique<ButtonAttachment> (apvts, prefix + "OneShot", oneShotButton);

    setupSlider (attackSlider, attackLabel, "Attack", 0.0, 5.0, 0.0);
    attackAtt = std::make_unique<SliderAttachment> (apvts, prefix + "Attack", attackSlider);

    setupSlider (decaySlider, decayLabel, "Decay", 0.0, 5.0, 0.0);
    decayAtt = std::make_unique<SliderAttachment> (apvts, prefix + "Decay", decaySlider);

    setupSlider (sustainSlider, sustainLabel, "Sustain", 0.0, 1.0, 1.0);
    sustainAtt = std::make_unique<SliderAttachment> (apvts, prefix + "Sustain", sustainSlider);

    setupSlider (releaseSlider, releaseLabel, "Release", 0.0, 5.0, 0.1);
    releaseAtt = std::make_unique<SliderAttachment> (apvts, prefix + "Release", releaseSlider);

    setupSlider (detuneSlider, detuneLabel, "Detune", -12.0, 12.0, 0.0);
    detuneSlider.setTextValueSuffix (" st");
    detuneAtt = std::make_unique<SliderAttachment> (apvts, prefix + "Detune", detuneSlider);
}

SampleGroupComponent::~SampleGroupComponent()
{
}

void SampleGroupComponent::setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, double min, double max, double def)
{
    addAndMakeVisible (label);
    label.setText (text, juce::NotificationType::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (12.0f);

    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange (min, max);
    slider.setValue (def);
    slider.setTooltip (text);
    slider.setLookAndFeel (&fxmeLookAndFeel);
    
    setSliderColours (slider, juce::Colours::cyan);
}

void SampleGroupComponent::paint (juce::Graphics& g)
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

void SampleGroupComponent::resized()
{
    auto area = getLocalBounds().reduced (5);
    
    auto headerArea = area.removeFromTop (25);
    nameLabel.setBounds (headerArea.removeFromLeft (area.getWidth() / 2));
    oneShotButton.setBounds (headerArea);
    
    auto detuneArea = area.removeFromTop (50);
    detuneLabel.setBounds (detuneArea.removeFromTop (15));
    detuneSlider.setBounds (detuneArea.reduced (2));

    // ADSR Grid
    auto adsrArea = area;
    int w = adsrArea.getWidth() / 4;
    
    auto setupBounds = [&](juce::Slider& s, juce::Label& l, int idx)
    {
        auto r = adsrArea.withX (adsrArea.getX() + idx * w).withWidth (w);
        l.setBounds (r.removeFromTop (15));
        s.setBounds (r.reduced (2));
    };

    setupBounds (attackSlider, attackLabel, 0);
    setupBounds (decaySlider, decayLabel, 1);
    setupBounds (sustainSlider, sustainLabel, 2);
    setupBounds (releaseSlider, releaseLabel, 3);
}
