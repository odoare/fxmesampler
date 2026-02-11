/*
  ==============================================================================

    MixerComponent.cpp

  ==============================================================================
*/

#include "MixerComponent.h"
#include "EffectChainReverbComponent.h"
#include "EffectChainDelay.h"
#include "EffectChainDelayComponent.h"
#include "EffectChainDynamicsComponent.h"
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

    // Create Send Sliders
    for (const auto& send : strip.getSends())
    {
        auto slider = std::make_unique<juce::Slider>();
        slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider->setRange (-60.0, 6.0);
        slider->setTooltip ("Send to " + send.busName);
        slider->setLookAndFeel(&fxmeLookAndFeel);
        setSliderColours (*slider, juce::Colours::cyan); // Use a distinct color for sends
        addAndMakeVisible (*slider);
        sendAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, strip.getName() + "_Send_" + send.busName, *slider));
        sendSliders.push_back (std::move (slider));
    }

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

    // Add sends to knobs area
    juce::FlexBox fbSends;
    fbSends.flexDirection = juce::FlexBox::Direction::row;
    fbSends.flexWrap = juce::FlexBox::Wrap::wrap;
    for (auto& s : sendSliders)
        fbSends.items.add(fi(*s).withFlex(0.0f).withWidth(30.0f).withHeight(30.0f).withMargin(2.0f));
    
    if (!sendSliders.empty())
        fbKnobs.items.add(fi(fbSends).withFlex(1.f));

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

    // Add sends
    juce::FlexBox fbSends;
    fbSends.flexDirection = juce::FlexBox::Direction::row;
    fbSends.flexWrap = juce::FlexBox::Wrap::wrap;
    for (auto& s : sendSliders)
        fbSends.items.add(fi(*s).withFlex(0.0f).withWidth(30.0f).withHeight(30.0f).withMargin(2.0f));
    if (!sendSliders.empty())
        fbKnobs.items.add(fi(fbSends).withFlex(1.f));

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

    // Add sends
    juce::FlexBox fbSends;
    fbSends.flexDirection = juce::FlexBox::Direction::row;
    fbSends.flexWrap = juce::FlexBox::Wrap::wrap;
    for (auto& s : sendSliders)
        fbSends.items.add(fi(*s).withFlex(0.0f).withWidth(30.0f).withHeight(30.0f).withMargin(2.0f));
    if (!sendSliders.empty())
        fbKnobs.items.add(fi(fbSends).withFlex(1.f));

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

    // Add sends
    juce::FlexBox fbSends;
    fbSends.flexDirection = juce::FlexBox::Direction::row;
    fbSends.flexWrap = juce::FlexBox::Wrap::wrap;
    for (auto& s : sendSliders)
        fbSends.items.add(fi(*s).withFlex(0.0f).withWidth(30.0f).withHeight(30.0f).withMargin(2.0f));
    if (!sendSliders.empty())
        fbMain.items.add(fi(fbSends).withFlex(0.2f));

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

    if (s.reverb.getImpulseNames().size() > 1)
    {
        addAndMakeVisible (irBox);
        const auto& names = s.reverb.getImpulseNames();
        for (int i = 0; i < names.size(); ++i)
            irBox.addItem (names[i], i + 1);
        
        irAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, s.getName() + "_IR", irBox);
    }

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
    
    if (irBox.isVisible())
        fbMain.items.add(fi(icon).withFlex(0.2f));
    else
        fbMain.items.add(fi(icon).withFlex(0.3f));

    if (irBox.isVisible())
        fbMain.items.add(fi(irBox).withFlex(0.1f).withMargin(2));

    // Add sends
    juce::FlexBox fbSends;
    fbSends.flexDirection = juce::FlexBox::Direction::row;
    fbSends.flexWrap = juce::FlexBox::Wrap::wrap;
    for (auto& s : sendSliders)
        fbSends.items.add(fi(*s).withFlex(0.0f).withWidth(30.0f).withHeight(30.0f).withMargin(2.0f));
    if (!sendSliders.empty())
        fbMain.items.add(fi(fbSends).withFlex(0.2f));

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

    if (s.reverb.getImpulseNames().size() > 1)
    {
        addAndMakeVisible (irBox);
        const auto& names = s.reverb.getImpulseNames();
        for (int i = 0; i < names.size(); ++i)
            irBox.addItem (names[i], i + 1);
        
        irAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, s.getName() + "_IR", irBox);
    }

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

    if (irBox.isVisible())
        fbMain.items.add(fi(icon).withFlex(0.2f));
    else
        fbMain.items.add(fi(icon).withFlex(0.3f));

    if (irBox.isVisible())
        fbMain.items.add(fi(irBox).withFlex(0.1f).withMargin(2));

    // Add sends
    juce::FlexBox fbSends;
    fbSends.flexDirection = juce::FlexBox::Direction::row;
    fbSends.flexWrap = juce::FlexBox::Wrap::wrap;
    for (auto& s : sendSliders)
        fbSends.items.add(fi(*s).withFlex(0.0f).withWidth(30.0f).withHeight(30.0f).withMargin(2.0f));
    if (!sendSliders.empty())
        fbMain.items.add(fi(fbSends).withFlex(0.2f));

    fbMain.items.add(fi(panSlider).withFlex(0.2f).withMargin(juce::FlexItem::Margin(10.f, 0.f, 0.f, 0.f)));
    fbMain.items.add(fi(fbSlider).withFlex(0.8f));

    fbMain.performLayout (bounds);
}

void MonoReverbStripComponent::updateMeters()
{
    meter.setValue (reverbStrip.meter.getRMS());
}

//==============================================================================
BusStripComponent::BusStripComponent (BusStrip& s, juce::AudioProcessorValueTreeState& apvts)
    : StripComponent (s, apvts), busStrip (s)
{
    setupKnob (panSlider, s.getName() + "_Pan", apvts);
    panAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Pan", panSlider);

    setupKnob (wSlider, s.getName() + "_Width", apvts);
    wAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Width", wSlider);

    juce::Colour c = s.getColor();
    if (c.isTransparent()) c = juce::Colours::purple;

    setSliderColours (panSlider, c);
    setSliderColours (wSlider, c);
    setSliderColours (levelSlider, c);

    addAndMakeVisible (meterL); meterL.setMeterColor (c);
    addAndMakeVisible (meterR); meterR.setMeterColor (c);
    meterL.setRange (-60.0f, 6.0f); meterL.setZeroLevel (0.0f);
    meterR.setRange (-60.0f, 6.0f); meterR.setZeroLevel (0.0f);
}

void BusStripComponent::resized()
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

    // Add sends
    juce::FlexBox fbSends;
    fbSends.flexDirection = juce::FlexBox::Direction::row;
    fbSends.flexWrap = juce::FlexBox::Wrap::wrap;
    for (auto& s : sendSliders)
        fbSends.items.add(fi(*s).withFlex(0.0f).withWidth(30.0f).withHeight(30.0f).withMargin(2.0f));
    if (!sendSliders.empty())
        fbKnobs.items.add(fi(fbSends).withFlex(1.f));

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

void BusStripComponent::updateMeters()
{
    meterL.setValue (busStrip.meterL.getRMS());
    meterR.setValue (busStrip.meterR.getRMS());
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

    // Add sends
    juce::FlexBox fbSends;
    fbSends.flexDirection = juce::FlexBox::Direction::row;
    fbSends.flexWrap = juce::FlexBox::Wrap::wrap;
    for (auto& s : sendSliders)
        fbSends.items.add(fi(*s).withFlex(0.0f).withWidth(30.0f).withHeight(30.0f).withMargin(2.0f));
    if (!sendSliders.empty())
        fbKnobs.items.add(fi(fbSends).withFlex(1.f));

    fbMain.items.add(fi(fbKnobs).withFlex(0.2f).withMargin(juce::FlexItem::Margin(10.f, 0.f, 0.f, 0.f)));
    fbMain.items.add(fi(fbSlider).withFlex(0.8f));

    fbMain.performLayout (bounds);
}

void MasterStripComponent::updateMeters()
{
    meterL.setValue (masterStrip.meterL.getRMS());
    meterR.setValue (masterStrip.meterR.getRMS());
}

void WelcomeComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    auto area = getLocalBounds().toFloat().reduced(20);
    
    if (img.isValid())
    {
        auto imgArea = area.removeFromTop(area.getHeight() * 0.7f);
        g.drawImage(img, imgArea, juce::RectanglePlacement::centred);
    }
    
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText(text, area.toNearestInt(), juce::Justification::centred, 10);
}

void WelcomeComponent::resized() {}

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
        else if (auto* s = dynamic_cast<BusStrip*>(strip.get()))
            this->strips.push_back (std::make_unique<BusStripComponent> (*s, state));
            
        if (!this->strips.empty() && this->strips.back()->getParentComponent() == nullptr)
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
    welcomeComp.setText(mixer.getWelcomeText());
    welcomeComp.setImage(mixer.getWelcomeImage());
    tabs.addTab("Welcome", juce::Colours::black, &welcomeComp, false);

    tabs.addTab ("Levels", juce::Colours::black, &levelsComp, false);
    
    // We create EffectChainComponents dynamically and pass ownership to the tabs
    // Note: In a real app we might want to store these pointers to avoid leaks if tabs are cleared,
    // but addTab takes ownership if we pass true (which we will for these new components).
    // However, addTab with 'true' deletes the component. We need to make sure we allocate them.
    
    auto addChainComponent = [&](const juce::String& name, EffectChain* chain) {
        if (auto* dynChain = dynamic_cast<EffectChainDynamics*>(chain))
        {
            tabs.addTab (name, juce::Colours::black, new EffectChainDynamicsComponent (*dynChain, apvts, name), true);
        }
        else if (auto* reverbChain = dynamic_cast<EffectChainReverb*>(chain))
        {
            tabs.addTab (name, juce::Colours::black, new EffectChainReverbComponent (*reverbChain, apvts, name), true);
        }
        else if (auto* delayChain = dynamic_cast<EffectChainDelay*>(chain))
        {
            tabs.addTab (name, juce::Colours::black, new EffectChainDelayComponent (*delayChain, apvts, name), true);
        }
    };

    for (auto& strip : mixer.getStrips())
    {
        addChainComponent (strip->getName(), strip->getEffectChain());
    }
    
    auto& master = mixer.getMasterStrip();
    addChainComponent (master.getName(), master.getEffectChain());

    tabs.addTab ("Sampler", juce::Colours::black, &samplerComp, false);

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