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
    addAndMakeVisible (vuLabel); vuLabel.setText ("Levels", juce::NotificationType::dontSendNotification);
    
    const auto& strips = mixer.getStrips();
    for (auto& strip : strips)
    {
        StripControls sc;
        sc.label = std::make_unique<juce::Label>();
        sc.label->setText (strip->getName(), juce::NotificationType::dontSendNotification);
        addAndMakeVisible (*sc.label);

        auto addSlider = [&](double min, double max, double def) {
            auto s = std::make_unique<juce::Slider>();
            s->setRange(min, max); s->setValue(def); s->addListener(this);
            addAndMakeVisible(*s);
            sc.sliders.push_back(std::move(s));
        };

        auto addMeter = [&](juce::Colour c) {
            auto m = std::make_unique<VuMeterComponent>();
            m->setMeterColor(c); m->setRange(-60.0f, 6.0f); m->setZeroLevel(0.0f);
            addAndMakeVisible(*m);
            sc.meters.push_back(std::move(m));
        };

        if (auto* s = dynamic_cast<AmbisonicStrip*>(strip.get()))
        {
            addSlider(-180, 180, 0); // Az
            addSlider(-90, 90, 0);   // El
            addSlider(0, 2, 1);      // Width
            addSlider(0, 2, 1);      // Level
            addMeter(juce::Colours::orange); // L
            addMeter(juce::Colours::orange); // R
        }
        else if (auto* s = dynamic_cast<StereoStrip*>(strip.get()))
        {
            addSlider(0, 2, 1);      // Width
            addSlider(0, 2, 1);      // Level
            addMeter(juce::Colours::cyan);
            addMeter(juce::Colours::cyan);
        }
        else if (auto* s = dynamic_cast<MonoStrip*>(strip.get()))
        {
            addSlider(0, 2, 1);      // Level
            addSlider(-1, 1, 0);     // Pan
            addMeter(juce::Colours::green);
        }

        stripControls.push_back(std::move(sc));
    }

    startTimerHz (24);
}

void MixerComponent::LevelsComponent::resized()
{
    int x = 10; int y = 10; int h = 20; int w = getWidth() - 20;
    int sliderH = 20;

    for (auto& sc : stripControls)
    {
        sc.label->setBounds(x, y, w, h); y += h;
        
        // Layout sliders
        for (auto& s : sc.sliders)
        {
            s->setBounds(x, y, w, sliderH);
            y += sliderH;
        }
        y += 5;
    }

    // Layout meters at bottom or side? Let's put them at the bottom row
    y += 10;
    vuLabel.setBounds(x, y, w, h); y += h;
    
    int meterW = 15; int meterH = 100; int gap = 5;
    int mx = x;
    
    for (auto& sc : stripControls)
    {
        for (auto& m : sc.meters)
        {
            m->setBounds(mx, y, meterW, meterH);
            mx += meterW + 2;
        }
        mx += gap;
    }
}

void MixerComponent::LevelsComponent::sliderValueChanged (juce::Slider* s)
{
    // Map slider back to strip parameter
    const auto& strips = mixer.getStrips();
    for (size_t i = 0; i < strips.size() && i < stripControls.size(); ++i)
    {
        auto& sc = stripControls[i];
        auto* strip = strips[i].get();
        
        if (auto* as = dynamic_cast<AmbisonicStrip*>(strip))
        {
            if (s == sc.sliders[0].get()) as->ambix.setAzimuth((float)s->getValue());
            if (s == sc.sliders[1].get()) as->ambix.setElevation((float)s->getValue());
            if (s == sc.sliders[2].get()) as->ambix.setWidth((float)s->getValue());
            if (s == sc.sliders[3].get()) as->ambix.setLevel((float)s->getValue());
        }
        else if (auto* ss = dynamic_cast<StereoStrip*>(strip))
        {
            if (s == sc.sliders[0].get()) ss->width = (float)s->getValue();
            if (s == sc.sliders[1].get()) ss->level = (float)s->getValue();
        }
        else if (auto* ms = dynamic_cast<MonoStrip*>(strip))
        {
            if (s == sc.sliders[0].get()) ms->level = (float)s->getValue();
            if (s == sc.sliders[1].get()) ms->pan = (float)s->getValue();
        }
    }
}

void MixerComponent::LevelsComponent::timerCallback()
{
    const auto& strips = mixer.getStrips();
    for (size_t i = 0; i < strips.size() && i < stripControls.size(); ++i)
    {
        auto& sc = stripControls[i];
        auto* strip = strips[i].get();

        if (auto* as = dynamic_cast<AmbisonicStrip*>(strip))
        {
            sc.meters[0]->setValue(as->meterL.getRMS());
            sc.meters[1]->setValue(as->meterR.getRMS());
        }
        else if (auto* ss = dynamic_cast<StereoStrip*>(strip))
        {
            sc.meters[0]->setValue(ss->meterL.getRMS());
            sc.meters[1]->setValue(ss->meterR.getRMS());
        }
        else if (auto* ms = dynamic_cast<MonoStrip*>(strip))
        {
            sc.meters[0]->setValue(ms->meter.getRMS());
        }
    }
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

    for (auto& strip : mixer.getStrips())
    {
        addChain (strip->getName(), strip->getEQ(), strip->getComp());
    }

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