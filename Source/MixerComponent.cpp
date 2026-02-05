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
        auto area = getLocalBounds();
        orderBox.setBounds (area.removeFromTop (25));
        eqComp.setBounds (area.removeFromTop (area.getHeight() / 2));
        compComp.setBounds (area.removeFromTop (area.getHeight() * 0.6f));
        tubeComp.setBounds (area);
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
    levelSlider.setSliderStyle (juce::Slider::LinearVertical);
    levelSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    levelSlider.setRange (0.0, 2.0);
    levelSlider.setValue (1.0);
    levelAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, strip.getName() + "_Level", levelSlider);

    addAndMakeVisible (muteButton);
    muteButton.setButtonText ("M");
    muteAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, strip.getName() + "_Mute", muteButton);

    addAndMakeVisible (soloButton);
    soloButton.setButtonText ("S");
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
    juce::ColourGradient grad (bluegreengrey.darker().darker().darker(), perpendicular * height,
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

//==============================================================================
AmbisonicStripComponent::AmbisonicStripComponent (AmbisonicStrip& s, juce::AudioProcessorValueTreeState& apvts)
    : StripComponent (s, apvts), ambStrip (s)
{
    setupKnob (azSlider, s.getName() + "_Azimuth", apvts);
    azAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Azimuth", azSlider);
    
    setupKnob (elSlider, s.getName() + "_Elevation", apvts);
    elAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Elevation", elSlider);
    
    setupKnob (wSlider, s.getName() + "_Width", apvts);
    wAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Width", wSlider);

    addAndMakeVisible (meterL); meterL.setMeterColor (juce::Colours::orange);
    addAndMakeVisible (meterR); meterR.setMeterColor (juce::Colours::orange);
    meterL.setRange (-60.0f, 6.0f); meterL.setZeroLevel (0.0f);
    meterR.setRange (-60.0f, 6.0f); meterR.setZeroLevel (0.0f);
}

void AmbisonicStripComponent::resized()
{
    auto area = getLocalBounds().reduced (2);
    nameLabel.setBounds (area.removeFromTop (20));
    
    auto knobs = area.removeFromTop (60);
    int kw = knobs.getWidth() / 2;
    azSlider.setBounds (knobs.removeFromLeft (kw).removeFromTop (30));
    elSlider.setBounds (knobs.removeFromLeft (kw).removeFromTop (30)); // Actually this logic is flawed for 2x2.
    
    // Let's do fixed layout for knobs
    int w = getWidth();
    azSlider.setBounds (0, 20, w/2, 40);
    elSlider.setBounds (w/2, 20, w/2, 40);
    wSlider.setBounds (w/4, 60, w/2, 40);
    
    area.removeFromTop (80); // Space used by knobs

    // Mute and Solo
    auto buttonArea = area.removeFromTop (20);
    muteButton.setBounds (buttonArea.removeFromLeft (buttonArea.getWidth() / 2));
    soloButton.setBounds (buttonArea);

    int iconHeight = area.getWidth();
    icon.setBounds (area.removeFromBottom (iconHeight));
    area.removeFromBottom (5);

    // Fader and Meters
    int meterW = 10;
    meterL.setBounds (area.getX(), area.getY(), meterW, area.getHeight());
    meterR.setBounds (area.getRight() - meterW, area.getY(), meterW, area.getHeight());
    levelSlider.setBounds (area.getX() + meterW, area.getY(), area.getWidth() - 2 * meterW, area.getHeight());
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

    addAndMakeVisible (meterL); meterL.setMeterColor (juce::Colours::cyan);
    addAndMakeVisible (meterR); meterR.setMeterColor (juce::Colours::cyan);
    meterL.setRange (-60.0f, 6.0f); meterL.setZeroLevel (0.0f);
    meterR.setRange (-60.0f, 6.0f); meterR.setZeroLevel (0.0f);
}

void MSStripComponent::resized()
{
    auto area = getLocalBounds().reduced (2);
    nameLabel.setBounds (area.removeFromTop (20));
    
    auto knobsArea = area.removeFromTop(50);
    auto panArea = knobsArea.removeFromLeft(knobsArea.getWidth() / 2);
    auto widthArea = knobsArea;
    panSlider.setBounds(panArea.withSizeKeepingCentre(40, 40));
    wSlider.setBounds(widthArea.withSizeKeepingCentre(40, 40));

    // Mute and Solo
    auto buttonArea = area.removeFromTop (20);
    muteButton.setBounds (buttonArea.removeFromLeft (buttonArea.getWidth() / 2));
    soloButton.setBounds (buttonArea);

    int iconHeight = area.getWidth();
    icon.setBounds (area.removeFromBottom (iconHeight));
    area.removeFromBottom (5);

    int meterW = 10;
    meterL.setBounds (area.getX(), area.getY(), meterW, area.getHeight());
    meterR.setBounds (area.getRight() - meterW, area.getY(), meterW, area.getHeight());
    levelSlider.setBounds (area.getX() + meterW, area.getY(), area.getWidth() - 2 * meterW, area.getHeight());
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

    addAndMakeVisible (meterL); meterL.setMeterColor (juce::Colours::cyan);
    addAndMakeVisible (meterR); meterR.setMeterColor (juce::Colours::cyan);
    meterL.setRange (-60.0f, 6.0f); meterL.setZeroLevel (0.0f);
    meterR.setRange (-60.0f, 6.0f); meterR.setZeroLevel (0.0f);
}

void StereoStripComponent::resized()
{
    auto area = getLocalBounds().reduced (2);
    nameLabel.setBounds (area.removeFromTop (20));
    
    auto knobsArea = area.removeFromTop(50);
    auto panArea = knobsArea.removeFromLeft(knobsArea.getWidth() / 2);
    auto widthArea = knobsArea;
    panSlider.setBounds(panArea.withSizeKeepingCentre(40, 40));
    wSlider.setBounds(widthArea.withSizeKeepingCentre(40, 40));

    // Mute and Solo
    auto buttonArea = area.removeFromTop (20);
    muteButton.setBounds (buttonArea.removeFromLeft (buttonArea.getWidth() / 2));
    soloButton.setBounds (buttonArea);

    int iconHeight = area.getWidth();
    icon.setBounds (area.removeFromBottom (iconHeight));
    area.removeFromBottom (5);

    int meterW = 10;
    meterL.setBounds (area.getX(), area.getY(), meterW, area.getHeight());
    meterR.setBounds (area.getRight() - meterW, area.getY(), meterW, area.getHeight());
    levelSlider.setBounds (area.getX() + meterW, area.getY(), area.getWidth() - 2 * meterW, area.getHeight());
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

    addAndMakeVisible (meter); meter.setMeterColor (juce::Colours::green);
    meter.setRange (-60.0f, 6.0f); meter.setZeroLevel (0.0f);
}

void MonoStripComponent::resized()
{
    auto area = getLocalBounds().reduced (2);
    nameLabel.setBounds (area.removeFromTop (20));
    
    panSlider.setBounds (area.removeFromTop (50).withSizeKeepingCentre (50, 50));

    // Mute and Solo
    auto buttonArea = area.removeFromTop (20);
    muteButton.setBounds (buttonArea.removeFromLeft (buttonArea.getWidth() / 2));
    soloButton.setBounds (buttonArea);

    int iconHeight = area.getWidth();
    icon.setBounds (area.removeFromBottom (iconHeight));
    area.removeFromBottom (5);

    int meterW = 10;
    meter.setBounds (area.getX(), area.getY(), meterW, area.getHeight());
    levelSlider.setBounds (area.getX() + meterW, area.getY(), area.getWidth() - meterW, area.getHeight());
}

void MonoStripComponent::updateMeters()
{
    meter.setValue (monoStrip.meter.getRMS());
}

//==============================================================================
MasterStripComponent::MasterStripComponent (MasterStrip& s, juce::AudioProcessorValueTreeState& apvts)
    : StripComponent (s, apvts), masterStrip (s)
{
    setupKnob (panSlider, s.getName() + "_Pan", apvts);
    panAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Pan", panSlider);

    setupKnob (wSlider, s.getName() + "_Width", apvts);
    wAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, s.getName() + "_Width", wSlider);

    addAndMakeVisible (meterL); meterL.setMeterColor (juce::Colours::red);
    addAndMakeVisible (meterR); meterR.setMeterColor (juce::Colours::red);
    meterL.setRange (-60.0f, 6.0f); meterL.setZeroLevel (0.0f);
    meterR.setRange (-60.0f, 6.0f); meterR.setZeroLevel (0.0f);
}

void MasterStripComponent::resized()
{
    auto area = getLocalBounds().reduced (2);
    nameLabel.setBounds (area.removeFromTop (20));
    
    auto knobsArea = area.removeFromTop(50);
    auto panArea = knobsArea.removeFromLeft(knobsArea.getWidth() / 2);
    auto widthArea = knobsArea;
    panSlider.setBounds(panArea.withSizeKeepingCentre(40, 40));
    wSlider.setBounds(widthArea.withSizeKeepingCentre(40, 40));

    auto buttonArea = area.removeFromTop (20);
    muteButton.setBounds (buttonArea.removeFromLeft (buttonArea.getWidth() / 2));
    // No solo for master

    int iconHeight = area.getWidth();
    icon.setBounds (area.removeFromBottom (iconHeight));
    area.removeFromBottom (5);

    int meterW = 10;
    meterL.setBounds (area.getX(), area.getY(), meterW, area.getHeight());
    meterR.setBounds (area.getRight() - meterW, area.getY(), meterW, area.getHeight());
    levelSlider.setBounds (area.getX() + meterW, area.getY(), area.getWidth() - 2 * meterW, area.getHeight());
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
MixerComponent::MixerComponent (Mixer& m, juce::AudioProcessorValueTreeState& state)
    : mixer (m), apvts (state), tabs (juce::TabbedButtonBar::TabsAtTop), levelsComp (m, state)
{
    tabs.addTab ("Levels", juce::Colours::lightgrey, &levelsComp, false);
    
    // We create EffectChainComponents dynamically and pass ownership to the tabs
    // Note: In a real app we might want to store these pointers to avoid leaks if tabs are cleared,
    // but addTab takes ownership if we pass true (which we will for these new components).
    // However, addTab with 'true' deletes the component. We need to make sure we allocate them.
    
    auto addChain = [&](const juce::String& name, Equalizer& eq, Compressor& comp, Tube& tube) {
        tabs.addTab (name, juce::Colours::lightgrey, new EffectChainComponent (eq, comp, tube, apvts, name), true);
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