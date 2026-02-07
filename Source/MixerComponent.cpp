/*
  ==============================================================================

    MixerComponent.cpp

  ==============================================================================
*/

#include "MixerComponent.h"

// Helper component to hold EQ and Compressor
class EffectChainComponent : public juce::Component
{
public:
    EffectChainComponent (Equalizer& eq, Compressor& comp, Tube& tube, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
        : eqComp (eq, apvts, prefix), compComp (comp, apvts, prefix), tubeComp (tube, apvts, prefix)
    {
        addAndMakeVisible (orderBox);
        orderBox.addItem ("EQ -> Comp -> Tube", 1);
        orderBox.addItem ("EQ -> Tube -> Comp", 2);
        orderBox.addItem ("Comp -> EQ -> Tube", 3);
        orderBox.addItem ("Comp -> Tube -> EQ", 4);
        orderBox.addItem ("Tube -> EQ -> Comp", 5);
        orderBox.addItem ("Tube -> Comp -> EQ", 6);
        orderAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, prefix + "_Order", orderBox);

        addAndMakeVisible (eqComp);
        addAndMakeVisible (compComp);
        addAndMakeVisible (tubeComp);
    }
    void resized() override
    {
        auto bounds = getLocalBounds();
        using fi = juce::FlexItem;
        juce::FlexBox fbMain, fb1, fb2;
        fbMain.flexDirection = juce::FlexBox::Direction::column;
        fb1.flexDirection = juce::FlexBox::Direction::column;
        fb2.flexDirection = juce::FlexBox::Direction::row;       
        fb1.items.add(fi(compComp).withFlex(1.2f).withMargin(juce::FlexItem::Margin(3.f, 3.f, 6.f, 6.f)));
        fb1.items.add(fi(tubeComp).withFlex(0.8f).withMargin(juce::FlexItem::Margin(3.f, 6.f, 6.f, 3.f)));
        fb2.items.add(fi(eqComp).withFlex(1.0f).withMargin(juce::FlexItem::Margin(6.f, 6.f, 3.f, 6.f)));
        fb2.items.add(fi(fb1).withFlex(1.0f));        
        fbMain.items.add(fi(orderBox).withFlex(0.1).withMargin(juce::FlexItem::Margin(6.f, 6.f, 3.f, 6.f)));
        fbMain.items.add(fi(fb2).withFlex(2.));
        fbMain.performLayout(bounds);

        // auto area = getLocalBounds();
        // orderBox.setBounds (area.removeFromTop (25));
        // eqComp.setBounds (area.removeFromTop (area.getHeight() / 2));
        // compComp.setBounds (area.removeFromTop (area.getHeight() * 0.6f));
        // tubeComp.setBounds (area);
    }
private:
    juce::ComboBox orderBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> orderAtt;
    EqualizerComponent eqComp;
    CompressorComponent compComp;
    TubeComponent tubeComp;
};

//==============================================================================
// StripComponent Base
StripComponent::StripComponent (MixerStrip& s, juce::AudioProcessorValueTreeState& apvts)
    : strip (s)
{
    addAndMakeVisible (nameLabel);
    nameLabel.setText (strip.getName(), juce::NotificationType::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (levelSlider);
    levelSlider.setSliderStyle (juce::Slider::LinearBarVertical);
    levelSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    levelSlider.setRange (-60.0, 6.0);
    levelSlider.setValue (0.0);
    levelSlider.setTextValueSuffix ("dB");
    levelSlider.setLookAndFeel(&fxmeLookAndFeel);
    levelAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, strip.getName() + "_Level", levelSlider);

    addAndMakeVisible (muteButton);
    muteButton.setButtonText ("M");
    muteButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::orange);
    muteButton.setLookAndFeel(&fxmeLookAndFeel);
    muteAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, strip.getName() + "_Mute", muteButton);

    addAndMakeVisible (soloButton);
    soloButton.setButtonText ("S");
    soloButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::green);
    soloButton.setLookAndFeel(&fxmeLookAndFeel);
    soloAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, strip.getName() + "_Solo", soloButton);

    addAndMakeVisible (icon);
    icon.setImage (strip.getImage());

    startTimerHz (24);
}

StripComponent::~StripComponent() { stopTimer(); }

void StripComponent::paint (juce::Graphics& g)
{
    auto diagonale = (getLocalBounds().getTopLeft() - getLocalBounds().getBottomRight()).toFloat();
    auto length = diagonale.getDistanceFromOrigin();
    auto perpendicular = diagonale.rotatedAboutOrigin (juce::degreesToRadians (270.0f)) / length;
    auto height = float (getWidth() * getHeight()) / length;
    auto bluegreengrey = juce::Colour::fromFloatRGBA (0.15f, 0.15f, 0.25f, 1.0f);
    juce::ColourGradient grad (bluegreengrey.darker().darker(), perpendicular * height,
                           bluegreengrey, perpendicular * -height, false);
    g.setGradientFill(grad);

    g.fillAll();
}

void StripComponent::resized()
{
    // Base resized not used, specific layouts in subclasses
}

void StripComponent::timerCallback()
{
    updateMeters();
}

void StripComponent::setupKnob (juce::Slider& s, const juce::String& paramID, juce::AudioProcessorValueTreeState& apvts)
{
    addAndMakeVisible (s);
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    // Attachments will handle range
    // But we can set default range just in case
    s.setRange (-1.0, 1.0); 
    // Store attachment in subclass
    s.setLookAndFeel (&fxmeLookAndFeel);
}

void StripComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

//==============================================================================
AmbisonicStripComponent::AmbisonicStripComponent (AmbisonicStrip& s, juce::AudioProcessorValueTreeState& apvts)
    : StripComponent (s, apvts), ambStrip (s)
{
    setupKnob (azSlider, s.getName() + "_Azimuth", apvts);
    azAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Azimuth", azSlider);
    azSlider.setTooltip ("Azimuth");
    
    setupKnob (elSlider, s.getName() + "_Elevation", apvts);
    elAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Elevation", elSlider);
    elSlider.setTooltip ("Elevation");
    
    setupKnob (wSlider, s.getName() + "_Width", apvts);
    wAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Width", wSlider);
    wSlider.setTooltip ("Width");

    juce::Colour c = s.getColor();
    if (c.isTransparent()) c = juce::Colours::orange;

    setSliderColours (azSlider, c);
    setSliderColours (elSlider, c);
    setSliderColours (wSlider, c);
    setSliderColours (levelSlider, c);

    addAndMakeVisible (meterL); meterL.setMeterColor (c);
    addAndMakeVisible (meterR); meterR.setMeterColor (c);
    meterL.setRange (-60.0f, 6.0f); meterL.setZeroLevel (0.0f);
    meterR.setRange (-60.0f, 6.0f); meterR.setZeroLevel (0.0f);
}

void AmbisonicStripComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem;
    juce::FlexBox fbMain, fbSlider, fbButtonsMeters, fbMeters, fbKnobs;

    fbMain.flexDirection = juce::FlexBox::Direction::column;
    fbSlider.flexDirection = juce::FlexBox::Direction::row;
    fbButtonsMeters.flexDirection = juce::FlexBox::Direction::column;
    fbMeters.flexDirection = juce::FlexBox::Direction::row;
    fbKnobs.flexDirection = juce::FlexBox::Direction::column;

    juce::FlexBox fbKnobsRow1;
    fbKnobsRow1.flexDirection = juce::FlexBox::Direction::row;
    fbKnobsRow1.items.add(fi(azSlider).withFlex(1.f));
    fbKnobsRow1.items.add(fi(elSlider).withFlex(1.f));

    fbKnobs.items.add(fi(fbKnobsRow1).withFlex(1.f));
    fbKnobs.items.add(fi(wSlider).withFlex(1.f));

    fbMeters.items.add(fi(meterL).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f, 5.f, 0, 0)));
    fbMeters.items.add(fi(meterR).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f, 0, 0, 5.f)));

    fbButtonsMeters.items.add(fi(muteButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(soloButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(fbMeters).withFlex(1.f));

    fbSlider.items.add(fi(levelSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 25.f, 10.f, 25.f)));
    fbSlider.items.add(fi(fbButtonsMeters).withFlex(1.f).withMargin(10.f));

    fbMain.items.add(fi(nameLabel).withFlex(0.1f));
    fbMain.items.add(fi(icon).withFlex(0.3f));
    fbMain.items.add(fi(fbKnobs).withFlex(0.3f).withMargin(juce::FlexItem::Margin(10.f, 0.f, 0.f, 0.f)));
    fbMain.items.add(fi(fbSlider).withFlex(0.8f));

    fbMain.performLayout (bounds);
}

void AmbisonicStripComponent::updateMeters()
{
    meterL.setValue (ambStrip.meterL.getRMS());
    meterR.setValue (ambStrip.meterR.getRMS());
}

//==============================================================================
MSStripComponent::MSStripComponent (MSStrip& s, juce::AudioProcessorValueTreeState& apvts)
    : StripComponent (s, apvts), msStrip (s)
{
    setupKnob (panSlider, s.getName() + "_Pan", apvts);
    panAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Pan", panSlider);

    setupKnob (wSlider, s.getName() + "_Width", apvts);
    wAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Width", wSlider);

    juce::Colour c = s.getColor();
    if (c.isTransparent()) c = juce::Colours::cyan;

    setSliderColours (panSlider, c);
    setSliderColours (wSlider, c);
    setSliderColours (levelSlider, c);

    addAndMakeVisible (meterL); meterL.setMeterColor (c);
    addAndMakeVisible (meterR); meterR.setMeterColor (c);
    meterL.setRange (-60.0f, 6.0f); meterL.setZeroLevel (0.0f);
    meterR.setRange (-60.0f, 6.0f); meterR.setZeroLevel (0.0f);
}

void MSStripComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem;
    juce::FlexBox fbMain, fbSlider, fbButtonsMeters, fbMeters, fbKnobs;

    fbMain.flexDirection = juce::FlexBox::Direction::column;
    fbSlider.flexDirection = juce::FlexBox::Direction::row;
    fbButtonsMeters.flexDirection = juce::FlexBox::Direction::column;
    fbMeters.flexDirection = juce::FlexBox::Direction::row;
    fbKnobs.flexDirection = juce::FlexBox::Direction::row;

    fbKnobs.items.add(fi(panSlider).withFlex(1.f));
    fbKnobs.items.add(fi(wSlider).withFlex(1.f));

    fbMeters.items.add(fi(meterL).withFlex(1.f).withMargin(juce::FlexItem::Margin(0, 2.f, 0, 0)));
    fbMeters.items.add(fi(meterR).withFlex(1.f).withMargin(juce::FlexItem::Margin(0, 0, 0, 2.f)));

    fbButtonsMeters.items.add(fi(muteButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(soloButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(fbMeters).withFlex(1.f));

    fbSlider.items.add(fi(levelSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 20.f, 10.f, 20.f)));
    fbSlider.items.add(fi(fbButtonsMeters).withFlex(1.f).withMargin(10.f));

    fbMain.items.add(fi(nameLabel).withFlex(0.1f));
    fbMain.items.add(fi(icon).withFlex(0.3f));
    fbMain.items.add(fi(fbKnobs).withFlex(0.2f));
    fbMain.items.add(fi(fbSlider).withFlex(0.8f));

    fbMain.performLayout (bounds);
}

void MSStripComponent::updateMeters()
{
    meterL.setValue (msStrip.meterL.getRMS());
    meterR.setValue (msStrip.meterR.getRMS());
}

//==============================================================================
StereoStripComponent::StereoStripComponent (StereoStrip& s, juce::AudioProcessorValueTreeState& apvts)
    : StripComponent (s, apvts), stereoStrip (s)
{
    setupKnob (panSlider, s.getName() + "_Pan", apvts);
    panAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Pan", panSlider);

    setupKnob (wSlider, s.getName() + "_Width", apvts);
    wAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Width", wSlider);

    juce::Colour c = s.getColor();
    if (c.isTransparent()) c = juce::Colours::cyan;

    setSliderColours (panSlider, c);
    setSliderColours (wSlider, c);
    setSliderColours (levelSlider, c);

    addAndMakeVisible (meterL); meterL.setMeterColor (c);
    addAndMakeVisible (meterR); meterR.setMeterColor (c);
    meterL.setRange (-60.0f, 6.0f); meterL.setZeroLevel (0.0f);
    meterR.setRange (-60.0f, 6.0f); meterR.setZeroLevel (0.0f);
}

void StereoStripComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem;
    juce::FlexBox fbMain, fbSlider, fbButtonsMeters, fbMeters, fbKnobs;

    fbMain.flexDirection = juce::FlexBox::Direction::column;
    fbSlider.flexDirection = juce::FlexBox::Direction::row;
    fbButtonsMeters.flexDirection = juce::FlexBox::Direction::column;
    fbMeters.flexDirection = juce::FlexBox::Direction::row;
    fbKnobs.flexDirection = juce::FlexBox::Direction::row;

    fbKnobs.items.add(fi(panSlider).withFlex(1.f));
    fbKnobs.items.add(fi(wSlider).withFlex(1.f));

    fbMeters.items.add(fi(meterL).withFlex(1.f).withMargin(juce::FlexItem::Margin(0, 2.f, 0, 0)));
    fbMeters.items.add(fi(meterR).withFlex(1.f).withMargin(juce::FlexItem::Margin(0, 0, 0, 2.f)));

    fbButtonsMeters.items.add(fi(muteButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(soloButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(fbMeters).withFlex(1.f));

    fbSlider.items.add(fi(levelSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 25.f, 10.f, 25.f)));
    fbSlider.items.add(fi(fbButtonsMeters).withFlex(1.f).withMargin(10.f));

    fbMain.items.add(fi(nameLabel).withFlex(0.1f));
    fbMain.items.add(fi(icon).withFlex(0.3f));
    fbMain.items.add(fi(fbKnobs).withFlex(0.2f));
    fbMain.items.add(fi(fbSlider).withFlex(0.8f));

    fbMain.performLayout (bounds);
}

void StereoStripComponent::updateMeters()
{
    meterL.setValue (stereoStrip.meterL.getRMS());
    meterR.setValue (stereoStrip.meterR.getRMS());
}

//==============================================================================
MonoStripComponent::MonoStripComponent (MonoStrip& s, juce::AudioProcessorValueTreeState& apvts)
    : StripComponent (s, apvts), monoStrip (s)
{
    setupKnob (panSlider, s.getName() + "_Pan", apvts);
    panAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Pan", panSlider);

    juce::Colour c = s.getColor();
    if (c.isTransparent()) c = juce::Colours::green;

    setSliderColours (panSlider, c);
    setSliderColours (levelSlider, c);

    addAndMakeVisible (meter); meter.setMeterColor (c);
    meter.setRange (-60.0f, 6.0f); meter.setZeroLevel (0.0f);
}

void MonoStripComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem; // Using alias for brevity
    juce::FlexBox fbMain, fbSlider, fbButtonsMeters, fbMeters;

    fbMain.flexDirection = juce::FlexBox::Direction::column;
    fbSlider.flexDirection = juce::FlexBox::Direction::row;
    fbButtonsMeters.flexDirection = juce::FlexBox::Direction::column;
    fbMeters.flexDirection = juce::FlexBox::Direction::row;

    fbMeters.items.add(fi(meter).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f,5.f,0.f,5.f)));
    fbButtonsMeters.items.add(fi(muteButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(soloButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(fbMeters).withFlex(1.f));
    fbSlider.items.add(fi(levelSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f,25.f,10.f,25.f)));
    fbSlider.items.add(fi(fbButtonsMeters).withFlex(1.f).withMargin(10.f));
    fbMain.items.add(fi(nameLabel).withFlex(0.1f));
    fbMain.items.add(fi(icon).withFlex(0.3f));
    fbMain.items.add(fi(panSlider).withFlex(0.2f).withMargin(juce::FlexItem::Margin(10.f,0.f,0.f,0.f)));
    fbMain.items.add(fi(fbSlider).withFlex(0.8f));

    fbMain.performLayout (bounds);

}

void MonoStripComponent::updateMeters()
{
    meter.setValue (monoStrip.meter.getRMS());
}

//==============================================================================
StereoReverbStripComponent::StereoReverbStripComponent (StereoReverbStrip& s, juce::AudioProcessorValueTreeState& apvts)
    : StripComponent (s, apvts), reverbStrip (s)
{
    setupKnob (panSlider, s.getName() + "_Pan", apvts);
    panAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Pan", panSlider);

    juce::Colour c = s.getColor();
    if (c.isTransparent()) c = juce::Colours::cyan;

    setSliderColours (panSlider, c);
    setSliderColours (levelSlider, c);

    addAndMakeVisible (meterL); meterL.setMeterColor (c);
    addAndMakeVisible (meterR); meterR.setMeterColor (c);
    meterL.setRange (-60.0f, 6.0f); meterL.setZeroLevel (0.0f);
    meterR.setRange (-60.0f, 6.0f); meterR.setZeroLevel (0.0f);
}

void StereoReverbStripComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem;
    juce::FlexBox fbMain, fbSlider, fbButtonsMeters, fbMeters;

    fbMain.flexDirection = juce::FlexBox::Direction::column;
    fbSlider.flexDirection = juce::FlexBox::Direction::row;
    fbButtonsMeters.flexDirection = juce::FlexBox::Direction::column;
    fbMeters.flexDirection = juce::FlexBox::Direction::row;

    fbMeters.items.add(fi(meterL).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f, 5.f, 0, 0)));
    fbMeters.items.add(fi(meterR).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f, 0, 0, 5.f)));

    fbButtonsMeters.items.add(fi(muteButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(soloButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(fbMeters).withFlex(1.f));

    fbSlider.items.add(fi(levelSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 25.f, 10.f, 25.f)));
    fbSlider.items.add(fi(fbButtonsMeters).withFlex(1.f).withMargin(10.f));

    fbMain.items.add(fi(nameLabel).withFlex(0.1f));
    fbMain.items.add(fi(icon).withFlex(0.3f));
    fbMain.items.add(fi(panSlider).withFlex(0.2f));
    fbMain.items.add(fi(fbSlider).withFlex(0.8f));

    fbMain.performLayout (bounds);
}

void StereoReverbStripComponent::updateMeters()
{
    meterL.setValue (reverbStrip.meterL.getRMS());
    meterR.setValue (reverbStrip.meterR.getRMS());
}

//==============================================================================
MonoReverbStripComponent::MonoReverbStripComponent (MonoReverbStrip& s, juce::AudioProcessorValueTreeState& apvts)
    : StripComponent (s, apvts), reverbStrip (s)
{
    setupKnob (panSlider, s.getName() + "_Pan", apvts);
    panAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Pan", panSlider);

    juce::Colour c = s.getColor();
    if (c.isTransparent()) c = juce::Colours::cyan;

    setSliderColours (panSlider, c);
    setSliderColours (levelSlider, c);

    addAndMakeVisible (meter); meter.setMeterColor (c);
    meter.setRange (-60.0f, 6.0f); meter.setZeroLevel (0.0f);
}

void MonoReverbStripComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem;
    juce::FlexBox fbMain, fbSlider, fbButtonsMeters, fbMeters;

    fbMain.flexDirection = juce::FlexBox::Direction::column;
    fbSlider.flexDirection = juce::FlexBox::Direction::row;
    fbButtonsMeters.flexDirection = juce::FlexBox::Direction::column;
    fbMeters.flexDirection = juce::FlexBox::Direction::row;

    fbMeters.items.add(fi(meter).withFlex(1.f).withMargin(10.f));
    fbButtonsMeters.items.add(fi(muteButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(soloButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(fbMeters).withFlex(1.f));
    fbSlider.items.add(fi(levelSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 25.f, 10.f, 25.f)));
    fbSlider.items.add(fi(fbButtonsMeters).withFlex(1.f).withMargin(10.f));
    fbMain.items.add(fi(nameLabel).withFlex(0.1f));
    fbMain.items.add(fi(icon).withFlex(0.3f));
    fbMain.items.add(fi(panSlider).withFlex(0.2f).withMargin(juce::FlexItem::Margin(10.f, 0.f, 0.f, 0.f)));
    fbMain.items.add(fi(fbSlider).withFlex(0.8f));

    fbMain.performLayout (bounds);
}

void MonoReverbStripComponent::updateMeters()
{
    meter.setValue (reverbStrip.meter.getRMS());
}

//==============================================================================
MasterStripComponent::MasterStripComponent (MasterStrip& s, juce::AudioProcessorValueTreeState& apvts)
    : StripComponent (s, apvts), masterStrip (s)
{
    setupKnob (panSlider, s.getName() + "_Pan", apvts);
    panAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Pan", panSlider);

    setupKnob (wSlider, s.getName() + "_Width", apvts);
    wAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Width", wSlider);

    juce::Colour c = s.getColor();
    if (c.isTransparent()) c = juce::Colours::red;

    setSliderColours (panSlider, c);
    setSliderColours (wSlider, c);
    setSliderColours (levelSlider, c);

    addAndMakeVisible (meterL); meterL.setMeterColor (c);
    addAndMakeVisible (meterR); meterR.setMeterColor (c);
    meterL.setRange (-60.0f, 6.0f); meterL.setZeroLevel (0.0f);
    meterR.setRange (-60.0f, 6.0f); meterR.setZeroLevel (0.0f);
}

void MasterStripComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem;
    juce::FlexBox fbMain, fbSlider, fbButtonsMeters, fbMeters, fbKnobs;

    fbMain.flexDirection = juce::FlexBox::Direction::column;
    fbSlider.flexDirection = juce::FlexBox::Direction::row;
    fbButtonsMeters.flexDirection = juce::FlexBox::Direction::column;
    fbMeters.flexDirection = juce::FlexBox::Direction::row;
    fbKnobs.flexDirection = juce::FlexBox::Direction::row;

    fbKnobs.items.add(fi(panSlider).withFlex(1.f));
    fbKnobs.items.add(fi(wSlider).withFlex(1.f));

    fbMeters.items.add(fi(meterL).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f, 2.f, 0, 0)));
    fbMeters.items.add(fi(meterR).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f, 0, 0, 2.f)));

    fbButtonsMeters.items.add(fi(muteButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi().withFlex(0.2f)); // Spacer for solo button
    fbButtonsMeters.items.add(fi(fbMeters).withFlex(1.f));

    fbSlider.items.add(fi(levelSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 25.f, 10.f, 25.f)));
    fbSlider.items.add(fi(fbButtonsMeters).withFlex(1.f).withMargin(10.f));

    fbMain.items.add(fi(nameLabel).withFlex(0.1f));
    fbMain.items.add(fi(icon).withFlex(0.3f));
    fbMain.items.add(fi(fbKnobs).withFlex(0.2f).withMargin(juce::FlexItem::Margin(10.f, 0.f, 0.f, 0.f)));
    fbMain.items.add(fi(fbSlider).withFlex(0.8f));

    fbMain.performLayout (bounds);
}

void MasterStripComponent::updateMeters()
{
    meterL.setValue (masterStrip.meterL.getRMS());
    meterR.setValue (masterStrip.meterR.getRMS());
}

//==============================================================================
MixerComponent::LevelsComponent::LevelsComponent (Mixer& m, juce::AudioProcessorValueTreeState& state)
    : mixer (m)
{
    const auto& strips = mixer.getStrips();
    for (auto& strip : strips)
    {
        if (auto* s = dynamic_cast<AmbisonicStrip*>(strip.get()))
            this->strips.push_back (std::make_unique<AmbisonicStripComponent> (*s, state));
        else if (auto* s = dynamic_cast<StereoStrip*>(strip.get()))
            this->strips.push_back (std::make_unique<StereoStripComponent> (*s, state));
        else if (auto* s = dynamic_cast<MSStrip*>(strip.get()))
            this->strips.push_back (std::make_unique<MSStripComponent> (*s, state));
        else if (auto* s = dynamic_cast<MonoStrip*>(strip.get()))
            this->strips.push_back (std::make_unique<MonoStripComponent> (*s, state));
        else if (auto* s = dynamic_cast<StereoReverbStrip*>(strip.get()))
            this->strips.push_back (std::make_unique<StereoReverbStripComponent> (*s, state));
        else if (auto* s = dynamic_cast<MonoReverbStrip*>(strip.get()))
            this->strips.push_back (std::make_unique<MonoReverbStripComponent> (*s, state));
            
        addAndMakeVisible (*this->strips.back());
    }

    this->strips.push_back (std::make_unique<MasterStripComponent> (mixer.getMasterStrip(), state));
    addAndMakeVisible (*this->strips.back());
}

void MixerComponent::LevelsComponent::resized()
{
    auto area = getLocalBounds().reduced (5);
    int stripWidth = area.getWidth() / (int) strips.size();
    
    for (auto& s : strips)
    {
        s->setBounds (area.removeFromLeft (stripWidth).reduced (2));
    }
}

//==============================================================================
MixerComponent::MixerComponent (Mixer& m, Sampler& s, juce::AudioProcessorValueTreeState& state)
    : mixer (m), apvts (state), tabs (juce::TabbedButtonBar::TabsAtTop), levelsComp (m, state), samplerComp (s, state)
{
    tabs.addTab ("Sampler", juce::Colours::black, &samplerComp, false);
    tabs.addTab ("Levels", juce::Colours::black, &levelsComp, false);
    
    // We create EffectChainComponents dynamically and pass ownership to the tabs
    // Note: In a real app we might want to store these pointers to avoid leaks if tabs are cleared,
    // but addTab takes ownership if we pass true (which we will for these new components).
    // However, addTab with 'true' deletes the component. We need to make sure we allocate them.
    
    auto addChain = [&](const juce::String& name, Equalizer& eq, Compressor& comp, Tube& tube) {
        tabs.addTab (name, juce::Colours::black, new EffectChainComponent (eq, comp, tube, apvts, name), true);
    };

    for (auto& strip : mixer.getStrips())
    {
        addChain (strip->getName(), strip->getEQ(), strip->getComp(), strip->getTube());
    }
    
    auto& master = mixer.getMasterStrip();
    addChain (master.getName(), master.getEQ(), master.getComp(), master.getTube());

    addAndMakeVisible (tabs);
    int n = mixer.getStrips().size() + 1; // +1 for Master
    setSize (n*150, 500);
}

MixerComponent::~MixerComponent()
{
}

void MixerComponent::paint (juce::Graphics& g)
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

void MixerComponent::resized()
{
    tabs.setBounds (getLocalBounds());
}