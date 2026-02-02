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
    EffectChainComponent (Equalizer& eq, Compressor& comp)
        : eqComp (eq), compComp (comp)
    {
        addAndMakeVisible (eqComp);
        addAndMakeVisible (compComp);
    }
    void resized() override
    {
        auto area = getLocalBounds();
        eqComp.setBounds (area.removeFromTop (area.getHeight() / 2));
        compComp.setBounds (area);
    }
private:
    EqualizerComponent eqComp;
    CompressorComponent compComp;
};

//==============================================================================
MixerComponent::LevelsComponent::LevelsComponent (Mixer& m) : mixer (m)
{
    auto setup = [&](juce::Slider& s, double min, double max, double def) {
        addAndMakeVisible (s); s.setRange (min, max); s.setValue (def); s.addListener (this);
    };

    addAndMakeVisible (ambLabel); ambLabel.setText ("Ambisonics (Az/El/W/Lvl)", juce::NotificationType::dontSendNotification);
    setup (azSlider, -180, 180, 0); setup (elSlider, -90, 90, 0); setup (wSlider, 0, 2, 1); setup (ambLvlSlider, 0, 2, 1);

    addAndMakeVisible (ambLvlLabel); ambLvlLabel.setText ("Ambience", juce::NotificationType::dontSendNotification);
    setup (ambLvlSlider2, 0, 2, 1);

    addAndMakeVisible (kickLabel); kickLabel.setText ("Kick", juce::NotificationType::dontSendNotification);
    setup (kickLvl, 0, 2, 1); setup (kickPan, -1, 1, 0);

    addAndMakeVisible (snareLabel); snareLabel.setText ("Snare", juce::NotificationType::dontSendNotification);
    setup (snareLvl, 0, 2, 1); setup (snarePan, -1, 1, 0);

    addAndMakeVisible (hhLabel); hhLabel.setText ("Hi-Hat", juce::NotificationType::dontSendNotification);
    setup (hhLvl, 0, 2, 1); setup (hhPan, -1, 1, 0);

    addAndMakeVisible (vuLabel); vuLabel.setText ("Levels", juce::NotificationType::dontSendNotification);
    
    auto setupVu = [&](VuMeterComponent& vu, juce::Colour c) { addAndMakeVisible(vu); vu.setMeterColor(c); vu.setRange(-60.0f, 6.0f); vu.setZeroLevel(0.0f); };
    setupVu(ambVuL, juce::Colours::orange);
    setupVu(ambVuR, juce::Colours::orange);
    setupVu(ambVu, juce::Colours::orange);
    setupVu(kickVu, juce::Colours::red);
    setupVu(snareVu, juce::Colours::yellow);
    setupVu(hhVu, juce::Colours::green);

    startTimerHz (24);
}

void MixerComponent::LevelsComponent::resized()
{
    int y = 10; int h = 20; int w = getWidth() - 20; int x = 10;
    
    ambLabel.setBounds (x, y, w, h); y += h;
    azSlider.setBounds (x, y, w, h); y += h;
    elSlider.setBounds (x, y, w, h); y += h;
    wSlider.setBounds (x, y, w, h); y += h;
    ambLvlSlider.setBounds (x, y, w, h); y += h + 10;

    ambLvlLabel.setBounds (x, y, w, h); y += h;
    ambLvlSlider2.setBounds (x, y, w, h); y += h + 10;

    auto track = [&](juce::Label& l, juce::Slider& vol, juce::Slider& pan) {
        l.setBounds (x, y, 60, h);
        vol.setBounds (x + 70, y, 150, h);
        pan.setBounds (x + 230, y, 150, h);
        y += h + 5;
    };
    track (kickLabel, kickLvl, kickPan);
    track (snareLabel, snareLvl, snarePan);
    track (hhLabel, hhLvl, hhPan);
    x = 10; y += 10;

    int vuW = 15; int vuH = 100; int gap = 20;
    vuLabel.setBounds (x, y, w, h); y += h;
    ambVuL.setBounds (x, y, vuW, vuH);
    ambVuR.setBounds (x + vuW + 2, y, vuW, vuH);
    ambVu.setBounds (x + 2 * vuW + 2 + gap, y, vuW, vuH);
    kickVu.setBounds (x + 3 * vuW + 2 + 2 * gap, y, vuW, vuH);
    snareVu.setBounds (x + 4 * vuW + 2 + 3 * gap, y, vuW, vuH);
    hhVu.setBounds (x + 5 * vuW + 2 + 4 * gap, y, vuW, vuH);
}

void MixerComponent::LevelsComponent::sliderValueChanged (juce::Slider* s)
{
    if (s == &azSlider) mixer.setAmbisonicAzimuth ((float) s->getValue());
    else if (s == &elSlider) mixer.setAmbisonicElevation ((float) s->getValue());
    else if (s == &wSlider) mixer.setAmbisonicWidth ((float) s->getValue());
    else if (s == &ambLvlSlider) mixer.setAmbisonicLevel ((float) s->getValue());
    else if (s == &ambLvlSlider2) mixer.setAmbienceLevel ((float) s->getValue());
    else if (s == &kickLvl) mixer.setKickLevel ((float) s->getValue());
    else if (s == &kickPan) mixer.setKickPan ((float) s->getValue());
    else if (s == &snareLvl) mixer.setSnareLevel ((float) s->getValue());
    else if (s == &snarePan) mixer.setSnarePan ((float) s->getValue());
    else if (s == &hhLvl) mixer.setHiHatLevel ((float) s->getValue());
    else if (s == &hhPan) mixer.setHiHatPan ((float) s->getValue());
}

void MixerComponent::LevelsComponent::timerCallback()
{
    ambVuL.setValue (mixer.getAmbisonicVuMeterL().getRMS());
    ambVuR.setValue (mixer.getAmbisonicVuMeterR().getRMS());
    ambVu.setValue (mixer.getAmbienceVuMeter().getRMS());
    kickVu.setValue (mixer.getKickVuMeter().getRMS());
    snareVu.setValue (mixer.getSnareVuMeter().getRMS());
    hhVu.setValue (mixer.getHiHatVuMeter().getRMS());
}

//==============================================================================
MixerComponent::MixerComponent (Mixer& m)
    : mixer (m), tabs (juce::TabbedButtonBar::TabsAtTop), levelsComp (m)
{
    tabs.addTab ("Levels", juce::Colours::lightgrey, &levelsComp, false);
    
    // We create EffectChainComponents dynamically and pass ownership to the tabs
    // Note: In a real app we might want to store these pointers to avoid leaks if tabs are cleared,
    // but addTab takes ownership if we pass true (which we will for these new components).
    // However, addTab with 'true' deletes the component. We need to make sure we allocate them.
    
    auto addChain = [&](const juce::String& name, Equalizer& eq, Compressor& comp) {
        tabs.addTab (name, juce::Colours::lightgrey, new EffectChainComponent (eq, comp), true);
    };

    addChain ("Ambisonics", mixer.getAmbisonicEqualizer(), mixer.getAmbisonicCompressor());
    addChain ("Ambience", mixer.getAmbienceEqualizer(), mixer.getAmbienceCompressor());
    addChain ("Kick", mixer.getKickEqualizer(), mixer.getKickCompressor());
    addChain ("Snare", mixer.getSnareEqualizer(), mixer.getSnareCompressor());
    addChain ("Hi-Hat", mixer.getHiHatEqualizer(), mixer.getHiHatCompressor());

    addAndMakeVisible (tabs);
    setSize (600, 500);
}

MixerComponent::~MixerComponent()
{
}

void MixerComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MixerComponent::resized()
{
    tabs.setBounds (getLocalBounds());
}