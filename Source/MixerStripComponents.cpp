/*
  ==============================================================================

    MixerStripComponents.cpp

  ==============================================================================
*/

#include "MixerStripComponents.h"

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
        auto slider = std::make_unique<FxmeSlider>();
        slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider->setRange (-60.0, 6.0);
        slider->setTooltip ("Send to " + send.busName);
        slider->setLookAndFeel(&fxmeLookAndFeel);
        setSliderColours (*slider, juce::Colours::cyan); // Use a distinct color for sends
        addAndMakeVisible (*slider);
        sendAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, strip.getName() + "_Send_" + send.busName, *slider));
        sendSliders.push_back (std::move (slider));

        auto button = std::make_unique<juce::ToggleButton>();
        button->setButtonText("Pre");
        button->setTooltip("Pre/Post FX+Fader");
        button->setColour(juce::ToggleButton::tickColourId, juce::Colours::white);
        button->setLookAndFeel(&fxmeLookAndFeel);
        addAndMakeVisible(*button);
        prePostAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, strip.getName() + "_Send_" + send.busName + "_Pre", *button));
        prePostButtons.push_back(std::move(button));
    }

    createRouteButtons(apvts);

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

void StripComponent::setStripColor(juce::Colour c)
{
    setSliderColours(levelSlider, c);
    for (auto& s : sendSliders)
        setSliderColours(*s, c);
    for (auto& b : prePostButtons)
        b->setColour(juce::ToggleButton::tickColourId, c);
    for (auto& b : routeButtons)
        b->setColour(juce::ToggleButton::tickColourId, c);
}

void StripComponent::createRouteButtons(juce::AudioProcessorValueTreeState& apvts)
{
    for (int i = 0; i < 4; ++i)
    {
        auto btn = std::make_unique<juce::ToggleButton>();
        juce::String name = (i == 0) ? "Main" : "Aux " + juce::String(i);
        juce::String paramId = (i == 0) ? strip.getName() + "_Route_Main" : strip.getName() + "_Route_Aux" + juce::String(i);
        
        btn->setButtonText(juce::String(i));
        btn->setTooltip("Route to " + name);
        btn->setColour(juce::ToggleButton::tickColourId, juce::Colours::white);
        btn->setLookAndFeel(&fxmeLookAndFeel);
        addAndMakeVisible(*btn);
        
        // Master strip doesn't have these params, so check if param exists
        if (apvts.getParameter(paramId) != nullptr)
            routeAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, paramId, *btn));
        
        routeButtons.push_back(std::move(btn));
    }
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

    setStripColor(c);
    setSliderColours (azSlider, c);
    setSliderColours (elSlider, c);
    setSliderColours (wSlider, c);

    addAndMakeVisible (meterL); meterL.setMeterColor (c);
    addAndMakeVisible (meterR); meterR.setMeterColor (c);
    meterL.setRange (-60.0f, 6.0f); meterL.setZeroLevel (0.0f);
    meterR.setRange (-60.0f, 6.0f); meterR.setZeroLevel (0.0f);
}

void AmbisonicStripComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem;
    juce::FlexBox fbMain, fbBottom, fbButtonsMeters, fbMeters, fbKnobs;

    fbMain.flexDirection = juce::FlexBox::Direction::column;
    fbBottom.flexDirection = juce::FlexBox::Direction::row;
    fbButtonsMeters.flexDirection = juce::FlexBox::Direction::column;
    fbMeters.flexDirection = juce::FlexBox::Direction::row;
    fbKnobs.flexDirection = juce::FlexBox::Direction::column;

    juce::FlexBox fbKnobsRow1;
    fbKnobsRow1.flexDirection = juce::FlexBox::Direction::row;
    fbKnobsRow1.items.add(fi(azSlider).withFlex(1.f));
    fbKnobsRow1.items.add(fi(elSlider).withFlex(1.f));

    fbKnobs.items.add(fi(fbKnobsRow1).withFlex(1.f));
    fbKnobs.items.add(fi(wSlider).withFlex(1.f));

    // Add sends
    juce::FlexBox fbSends, fbOutputs;
    fbSends.flexDirection = juce::FlexBox::Direction::column;
    fbOutputs.flexDirection = juce::FlexBox::Direction::column;
    for (size_t i = 0; i < sendSliders.size(); ++i)
    {
        fbSends.items.add(fi(*sendSliders[i]).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0.f, -10.f, 0.f, -10.f)));
        fbSends.items.add(fi(*prePostButtons[i]).withFlex(0.5f).withMargin(juce::FlexItem::Margin(0.f, -10.f, 0.f, -10.f)));
    }
    fbSends.items.add(fi().withFlex(1.f));

    // Add routing buttons
    for (auto& b : routeButtons)
        fbOutputs.items.add(fi(*b).withFlex(1.0f));
    juce::FlexBox fbRoutes;
    fbRoutes.flexDirection = juce::FlexBox::Direction::column;
    fbRoutes.items.add(fi(fbSends).withFlex(1.f));
    fbRoutes.items.add(fi(fbOutputs).withFlex(1.f));

    fbMeters.items.add(fi(meterL).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f, 5.f, 0, 0)));
    fbMeters.items.add(fi(meterR).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f, 0, 0, 5.f)));

    fbButtonsMeters.items.add(fi(muteButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(soloButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(fbMeters).withFlex(1.f));

    fbBottom.items.add(fi(levelSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 10.f, 10.f, 10.f)));
    fbBottom.items.add(fi(fbRoutes).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 3.f, 10.f, 3.f)));
    fbBottom.items.add(fi(fbButtonsMeters).withFlex(1.f).withMargin(10.f));

    fbMain.items.add(fi(nameLabel).withFlex(0.1f));
    fbMain.items.add(fi(icon).withFlex(0.3f));
    fbMain.items.add(fi(fbKnobs).withFlex(0.3f).withMargin(juce::FlexItem::Margin(10.f, 0.f, 0.f, 0.f)));
    fbMain.items.add(fi(fbBottom).withFlex(0.8f));

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

    setStripColor(c);
    setSliderColours (panSlider, c);
    setSliderColours (wSlider, c);

    addAndMakeVisible (meterL); meterL.setMeterColor (c);
    addAndMakeVisible (meterR); meterR.setMeterColor (c);
    meterL.setRange (-60.0f, 6.0f); meterL.setZeroLevel (0.0f);
    meterR.setRange (-60.0f, 6.0f); meterR.setZeroLevel (0.0f);
}

void MSStripComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem;
    juce::FlexBox fbMain, fbBottom, fbButtonsMeters, fbMeters, fbKnobs;

    fbMain.flexDirection = juce::FlexBox::Direction::column;
    fbBottom.flexDirection = juce::FlexBox::Direction::row;
    fbButtonsMeters.flexDirection = juce::FlexBox::Direction::column;
    fbMeters.flexDirection = juce::FlexBox::Direction::row;
    fbKnobs.flexDirection = juce::FlexBox::Direction::row;

    fbKnobs.items.add(fi(panSlider).withFlex(1.f));
    fbKnobs.items.add(fi(wSlider).withFlex(1.f));

    // Add sends
    juce::FlexBox fbSends, fbOutputs;
    fbSends.flexDirection = juce::FlexBox::Direction::column;
    fbOutputs.flexDirection = juce::FlexBox::Direction::column;
    for (size_t i = 0; i < sendSliders.size(); ++i)
    {
        fbSends.items.add(fi(*sendSliders[i]).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0.f, -10.f, 0.f, -10.f)));
        fbSends.items.add(fi(*prePostButtons[i]).withFlex(0.5f).withMargin(juce::FlexItem::Margin(0.f, -10.f, 0.f, -10.f)));
    }
    fbSends.items.add(fi().withFlex(1.f));

    // Add routing buttons
    for (auto& b : routeButtons)
        fbOutputs.items.add(fi(*b).withFlex(1.0f));
    juce::FlexBox fbRoutes;
    fbRoutes.flexDirection = juce::FlexBox::Direction::column;
    fbRoutes.items.add(fi(fbSends).withFlex(1.f));
    fbRoutes.items.add(fi(fbOutputs).withFlex(1.f));

    fbMeters.items.add(fi(meterL).withFlex(1.f).withMargin(juce::FlexItem::Margin(0, 2.f, 0, 0)));
    fbMeters.items.add(fi(meterR).withFlex(1.f).withMargin(juce::FlexItem::Margin(0, 0, 0, 2.f)));

    fbButtonsMeters.items.add(fi(muteButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(soloButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(fbMeters).withFlex(1.f));

    fbBottom.items.add(fi(levelSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 10.f, 10.f, 10.f)));
    fbBottom.items.add(fi(fbRoutes).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 3.f, 10.f, 3.f)));
    fbBottom.items.add(fi(fbButtonsMeters).withFlex(1.f).withMargin(10.f));

    fbMain.items.add(fi(nameLabel).withFlex(0.1f));
    fbMain.items.add(fi(icon).withFlex(0.3f));
    fbMain.items.add(fi(fbKnobs).withFlex(0.2f));
    fbMain.items.add(fi(fbBottom).withFlex(0.8f));

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

    setStripColor(c);
    setSliderColours (panSlider, c);
    setSliderColours (wSlider, c);

    addAndMakeVisible (meterL); meterL.setMeterColor (c);
    addAndMakeVisible (meterR); meterR.setMeterColor (c);
    meterL.setRange (-60.0f, 6.0f); meterL.setZeroLevel (0.0f);
    meterR.setRange (-60.0f, 6.0f); meterR.setZeroLevel (0.0f);
}

void StereoStripComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem;
    juce::FlexBox fbMain, fbBottom, fbButtonsMeters, fbMeters, fbKnobs;

    fbMain.flexDirection = juce::FlexBox::Direction::column;
    fbBottom.flexDirection = juce::FlexBox::Direction::row;
    fbButtonsMeters.flexDirection = juce::FlexBox::Direction::column;
    fbMeters.flexDirection = juce::FlexBox::Direction::row;
    fbKnobs.flexDirection = juce::FlexBox::Direction::row;

    fbKnobs.items.add(fi(panSlider).withFlex(1.f));
    fbKnobs.items.add(fi(wSlider).withFlex(1.f));

    // Add sends
    juce::FlexBox fbSends, fbOutputs;
    fbSends.flexDirection = juce::FlexBox::Direction::column;
    fbOutputs.flexDirection = juce::FlexBox::Direction::column;
    // fbSends.flexWrap = juce::FlexBox::Wrap::wrap;
    for (size_t i = 0; i < sendSliders.size(); ++i)
    {
        fbSends.items.add(fi(*sendSliders[i]).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0.f, -10.f, 0.f, -10.f)));
        fbSends.items.add(fi(*prePostButtons[i]).withFlex(0.5f).withMargin(juce::FlexItem::Margin(0.f, -10.f, 0.f, -10.f)));
    }
    fbSends.items.add(fi().withFlex(1.f));

    // Add routing buttons
    for (auto& b : routeButtons)
        fbOutputs.items.add(fi(*b).withFlex(1.0f));
    juce::FlexBox fbRoutes;
    fbRoutes.flexDirection = juce::FlexBox::Direction::column;
    fbRoutes.items.add(fi(fbSends).withFlex(1.f));
    fbRoutes.items.add(fi(fbOutputs).withFlex(1.f));
    
    fbMeters.items.add(fi(meterL).withFlex(1.f).withMargin(juce::FlexItem::Margin(0, 2.f, 0, 0)));
    fbMeters.items.add(fi(meterR).withFlex(1.f).withMargin(juce::FlexItem::Margin(0, 0, 0, 2.f)));

    fbButtonsMeters.items.add(fi(muteButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(soloButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(fbMeters).withFlex(1.f));

    fbBottom.items.add(fi(levelSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 10.f, 10.f, 10.f)));
    fbBottom.items.add(fi(fbRoutes).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 3.f, 10.f, 3.f)));
    fbBottom.items.add(fi(fbButtonsMeters).withFlex(1.f).withMargin(10.f));

    fbMain.items.add(fi(nameLabel).withFlex(0.1f));
    fbMain.items.add(fi(icon).withFlex(0.3f));
    fbMain.items.add(fi(fbKnobs).withFlex(0.2f));
    fbMain.items.add(fi(fbBottom).withFlex(0.8f));

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

    setStripColor(c);
    setSliderColours (panSlider, c);

    addAndMakeVisible (meter); meter.setMeterColor (c);
    meter.setRange (-60.0f, 6.0f); meter.setZeroLevel (0.0f);
}

void MonoStripComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem; // Using alias for brevity
    juce::FlexBox fbMain, fbBottom, fbButtonsMeters, fbMeters, fbKnobs;

    fbMain.flexDirection = juce::FlexBox::Direction::column;
    fbBottom.flexDirection = juce::FlexBox::Direction::row;
    fbButtonsMeters.flexDirection = juce::FlexBox::Direction::column;
    fbMeters.flexDirection = juce::FlexBox::Direction::row;
    fbKnobs.flexDirection = juce::FlexBox::Direction::row;

    fbKnobs.items.add(fi(panSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f,0.f,0.f,0.f)));

    fbMeters.items.add(fi(meter).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f,5.f,0.f,5.f)));
    fbButtonsMeters.items.add(fi(muteButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(soloButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(fbMeters).withFlex(1.f));

    // Add sends
    juce::FlexBox fbSends, fbOutputs;
    fbSends.flexDirection = juce::FlexBox::Direction::column;
    fbOutputs.flexDirection = juce::FlexBox::Direction::column;
    for (size_t i = 0; i < sendSliders.size(); ++i)
    {
        fbSends.items.add(fi(*sendSliders[i]).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0.f, -10.f, 0.f, -10.f)));
        fbSends.items.add(fi(*prePostButtons[i]).withFlex(0.5f).withMargin(juce::FlexItem::Margin(0.f, -10.f, 0.f, -10.f)));
    }
    fbSends.items.add(fi().withFlex(1.f));

    // Add routing buttons
    for (auto& b : routeButtons)
        fbOutputs.items.add(fi(*b).withFlex(1.0f));
    juce::FlexBox fbRoutes;
    fbRoutes.flexDirection = juce::FlexBox::Direction::column;
    fbRoutes.items.add(fi(fbSends).withFlex(1.f));
    fbRoutes.items.add(fi(fbOutputs).withFlex(1.f));

    fbBottom.items.add(fi(levelSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 10.f, 10.f, 10.f)));
    fbBottom.items.add(fi(fbRoutes).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 3.f, 10.f, 3.f)));
    fbBottom.items.add(fi(fbButtonsMeters).withFlex(1.f).withMargin(10.f));

    fbMain.items.add(fi(nameLabel).withFlex(0.1f));
    fbMain.items.add(fi(icon).withFlex(0.3f));
    fbMain.items.add(fi(fbKnobs).withFlex(0.2f));
    fbMain.items.add(fi(fbBottom).withFlex(0.8f));

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
            irBox.addItem (juce::File (names[i]).getFileNameWithoutExtension(), i + 1);
        
        irAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, s.getName() + "_Rev_IR", irBox);
    }

    setStripColor(c);
    setSliderColours (panSlider, c);

    addAndMakeVisible (meterL); meterL.setMeterColor (c);
    addAndMakeVisible (meterR); meterR.setMeterColor (c);
    meterL.setRange (-60.0f, 6.0f); meterL.setZeroLevel (0.0f);
    meterR.setRange (-60.0f, 6.0f); meterR.setZeroLevel (0.0f);
}

void StereoReverbStripComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem;
    juce::FlexBox fbMain, fbBottom, fbButtonsMeters, fbMeters, fbKnobs;

    fbMain.flexDirection = juce::FlexBox::Direction::column;
    fbBottom.flexDirection = juce::FlexBox::Direction::row;
    fbButtonsMeters.flexDirection = juce::FlexBox::Direction::column;
    fbMeters.flexDirection = juce::FlexBox::Direction::row;
    fbKnobs.flexDirection = juce::FlexBox::Direction::row;

    fbKnobs.items.add(fi(panSlider).withFlex(1.f));

    fbMeters.items.add(fi(meterL).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f, 5.f, 0, 0)));
    fbMeters.items.add(fi(meterR).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f, 0, 0, 5.f)));

    fbButtonsMeters.items.add(fi(muteButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(soloButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(fbMeters).withFlex(1.f));


    fbMain.items.add(fi(nameLabel).withFlex(0.1f));
    
    if (irBox.isVisible())
        fbMain.items.add(fi(icon).withFlex(0.24f));
    else
        fbMain.items.add(fi(icon).withFlex(0.3f));

    if (irBox.isVisible())
        fbMain.items.add(fi(irBox).withFlex(0.06f).withMargin(2));

    // Add sends
    juce::FlexBox fbSends, fbOutputs;
    fbSends.flexDirection = juce::FlexBox::Direction::column;
    fbOutputs.flexDirection = juce::FlexBox::Direction::column;
    for (size_t i = 0; i < sendSliders.size(); ++i)
    {
        fbSends.items.add(fi(*sendSliders[i]).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0.f, -10.f, 0.f, -10.f)));
        fbSends.items.add(fi(*prePostButtons[i]).withFlex(0.5f).withMargin(juce::FlexItem::Margin(0.f, -10.f, 0.f, -10.f)));
    }
    fbSends.items.add(fi().withFlex(1.f));

    // Add routing buttons
    for (auto& b : routeButtons)
        fbOutputs.items.add(fi(*b).withFlex(1.0f));
    juce::FlexBox fbRoutes;
    fbRoutes.flexDirection = juce::FlexBox::Direction::column;
    fbRoutes.items.add(fi(fbSends).withFlex(1.f));
    fbRoutes.items.add(fi(fbOutputs).withFlex(1.f));

    fbBottom.items.add(fi(levelSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 10.f, 10.f, 10.f)));
    fbBottom.items.add(fi(fbRoutes).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 3.f, 10.f, 3.f)));
    fbBottom.items.add(fi(fbButtonsMeters).withFlex(1.f).withMargin(10.f));

    fbMain.items.add(fi(fbKnobs).withFlex(0.2f));
    fbMain.items.add(fi(fbBottom).withFlex(0.8f));

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
            irBox.addItem (juce::File (names[i]).getFileNameWithoutExtension(), i + 1);
        
        irAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, s.getName() + "_Rev_IR", irBox);
    }

    setStripColor(c);
    setSliderColours (panSlider, c);

    addAndMakeVisible (meter); meter.setMeterColor (c);
    meter.setRange (-60.0f, 6.0f); meter.setZeroLevel (0.0f);
}

void MonoReverbStripComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem;
    juce::FlexBox fbMain, fbBottom, fbButtonsMeters, fbMeters, fbKnobs;

    fbMain.flexDirection = juce::FlexBox::Direction::column;
    fbBottom.flexDirection = juce::FlexBox::Direction::row;
    fbButtonsMeters.flexDirection = juce::FlexBox::Direction::column;
    fbMeters.flexDirection = juce::FlexBox::Direction::row;
    fbKnobs.flexDirection = juce::FlexBox::Direction::row;

    fbKnobs.items.add(fi(panSlider).withFlex(1.f));

    fbMeters.items.add(fi(meter).withFlex(1.f).withMargin(10.f));
    fbButtonsMeters.items.add(fi(muteButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(soloButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(fbMeters).withFlex(1.f));
    fbMain.items.add(fi(nameLabel).withFlex(0.1f));

    if (irBox.isVisible())
        fbMain.items.add(fi(icon).withFlex(0.24f));
    else
        fbMain.items.add(fi(icon).withFlex(0.3f));

    if (irBox.isVisible())
        fbMain.items.add(fi(irBox).withFlex(0.06f).withMargin(2));

    // Add sends
    juce::FlexBox fbSends, fbOutputs;
    fbSends.flexDirection = juce::FlexBox::Direction::column;
    fbOutputs.flexDirection = juce::FlexBox::Direction::column;
    for (size_t i = 0; i < sendSliders.size(); ++i)
    {
        fbSends.items.add(fi(*sendSliders[i]).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0.f, -10.f, 0.f, -10.f)));
        fbSends.items.add(fi(*prePostButtons[i]).withFlex(0.5f).withMargin(juce::FlexItem::Margin(0.f, -10.f, 0.f, -10.f)));
    }
    fbSends.items.add(fi().withFlex(1.f));

    // Add routing buttons
    for (auto& b : routeButtons)
        fbOutputs.items.add(fi(*b).withFlex(1.0f));
    juce::FlexBox fbRoutes;
    fbRoutes.flexDirection = juce::FlexBox::Direction::column;
    fbRoutes.items.add(fi(fbSends).withFlex(1.f));
    fbRoutes.items.add(fi(fbOutputs).withFlex(1.f));

    fbBottom.items.add(fi(levelSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 10.f, 10.f, 10.f)));
    fbBottom.items.add(fi(fbRoutes).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 3.f, 10.f, 3.f)));
    fbBottom.items.add(fi(fbButtonsMeters).withFlex(1.f).withMargin(10.f));

    fbMain.items.add(fi(fbKnobs).withFlex(0.2f));
    fbMain.items.add(fi(fbBottom).withFlex(0.8f));

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

    setStripColor(c);
    setSliderColours (panSlider, c);
    setSliderColours (wSlider, c);

    addAndMakeVisible (meterL); meterL.setMeterColor (c);
    addAndMakeVisible (meterR); meterR.setMeterColor (c);
    meterL.setRange (-60.0f, 6.0f); meterL.setZeroLevel (0.0f);
    meterR.setRange (-60.0f, 6.0f); meterR.setZeroLevel (0.0f);
}

void BusStripComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem;
    juce::FlexBox fbMain, fbBottom, fbButtonsMeters, fbMeters, fbKnobs;

    fbMain.flexDirection = juce::FlexBox::Direction::column;
    fbBottom.flexDirection = juce::FlexBox::Direction::row;
    fbButtonsMeters.flexDirection = juce::FlexBox::Direction::column;
    fbMeters.flexDirection = juce::FlexBox::Direction::row;
    fbKnobs.flexDirection = juce::FlexBox::Direction::row;

    fbKnobs.items.add(fi(panSlider).withFlex(1.f));
    fbKnobs.items.add(fi(wSlider).withFlex(1.f));

    // Add sends
    juce::FlexBox fbSends, fbOutputs;
    fbSends.flexDirection = juce::FlexBox::Direction::column;
    fbOutputs.flexDirection = juce::FlexBox::Direction::column;
    for (size_t i = 0; i < sendSliders.size(); ++i)
    {
        fbSends.items.add(fi(*sendSliders[i]).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0.f, -10.f, 0.f, -10.f)));
        fbSends.items.add(fi(*prePostButtons[i]).withFlex(0.5f).withMargin(juce::FlexItem::Margin(0.f, -10.f, 0.f, -10.f)));
    }
    fbSends.items.add(fi().withFlex(1.f));

    // Add routing buttons
    for (auto& b : routeButtons)
        fbOutputs.items.add(fi(*b).withFlex(1.0f));
    juce::FlexBox fbRoutes;
    fbRoutes.flexDirection = juce::FlexBox::Direction::column;
    fbRoutes.items.add(fi(fbSends).withFlex(1.f));
    fbRoutes.items.add(fi(fbOutputs).withFlex(1.f));

    fbMeters.items.add(fi(meterL).withFlex(1.f).withMargin(juce::FlexItem::Margin(0, 2.f, 0, 0)));
    fbMeters.items.add(fi(meterR).withFlex(1.f).withMargin(juce::FlexItem::Margin(0, 0, 0, 2.f)));

    fbButtonsMeters.items.add(fi(muteButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(soloButton).withFlex(0.2f));
    fbButtonsMeters.items.add(fi(fbMeters).withFlex(1.f));

    fbBottom.items.add(fi(levelSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 10.f, 10.f, 10.f)));
    fbBottom.items.add(fi(fbRoutes).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 3.f, 10.f, 3.f)));
    fbBottom.items.add(fi(fbButtonsMeters).withFlex(1.f).withMargin(10.f));

    fbMain.items.add(fi(nameLabel).withFlex(0.1f));
    fbMain.items.add(fi(icon).withFlex(0.3f));
    fbMain.items.add(fi(fbKnobs).withFlex(0.2f));
    fbMain.items.add(fi(fbBottom).withFlex(0.8f));

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

    setStripColor(c);
    setSliderColours (panSlider, c);
    setSliderColours (wSlider, c);

    addAndMakeVisible (meterL); meterL.setMeterColor (c);
    addAndMakeVisible (meterR); meterR.setMeterColor (c);
    meterL.setRange (-60.0f, 6.0f); meterL.setZeroLevel (0.0f);
    meterR.setRange (-60.0f, 6.0f); meterR.setZeroLevel (0.0f);
}

void MasterStripComponent::resized()
{
    auto bounds = getLocalBounds();
    using fi = juce::FlexItem;
    juce::FlexBox fbMain, fbBottom, fbButtonsMeters, fbMeters, fbKnobs;

    fbMain.flexDirection = juce::FlexBox::Direction::column;
    fbBottom.flexDirection = juce::FlexBox::Direction::row;
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


    // Add sends
    juce::FlexBox fbSends, fbOutputs;
    fbSends.flexDirection = juce::FlexBox::Direction::column;
    fbOutputs.flexDirection = juce::FlexBox::Direction::column;
    for (size_t i = 0; i < sendSliders.size(); ++i)
    {
        fbSends.items.add(fi(*sendSliders[i]).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0.f, -10.f, 0.f, -10.f)));
        fbSends.items.add(fi(*prePostButtons[i]).withFlex(0.5f).withMargin(juce::FlexItem::Margin(0.f, -10.f, 0.f, -10.f)));
    }
    fbSends.items.add(fi().withFlex(1.f));

    juce::FlexBox fbRoutes;
    fbRoutes.flexDirection = juce::FlexBox::Direction::column;
    fbRoutes.items.add(fi(fbSends).withFlex(1.f));
    fbRoutes.items.add(fi(fbOutputs).withFlex(1.f));

    fbBottom.items.add(fi(levelSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 10.f, 10.f, 10.f)));
    fbBottom.items.add(fi(fbRoutes).withFlex(1.f).withMargin(juce::FlexItem::Margin(10.f, 3.f, 10.f, 3.f)));
    fbBottom.items.add(fi(fbButtonsMeters).withFlex(1.f).withMargin(10.f));

    fbMain.items.add(fi(nameLabel).withFlex(0.1f));
    fbMain.items.add(fi(icon).withFlex(0.3f));
    fbMain.items.add(fi(fbKnobs).withFlex(0.2f).withMargin(juce::FlexItem::Margin(10.f, 0.f, 0.f, 0.f)));
    fbMain.items.add(fi(fbBottom).withFlex(0.8f));

    fbMain.performLayout (bounds);
}

void MasterStripComponent::updateMeters()
{
    meterL.setValue (masterStrip.meterL.getRMS());
    meterR.setValue (masterStrip.meterR.getRMS());
}