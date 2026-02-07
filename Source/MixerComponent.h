/*
  ==============================================================================

    MixerComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Mixer.h"
#include "CompressorComponent.h"
#include "VuMeterComponent.h"
#include "EqualizerComponent.h"
#include "TubeComponent.h"
#include "SamplerComponent.h"

//==============================================================================
class StripComponent : public juce::Component, public juce::Timer
{
public:
    StripComponent (MixerStrip& strip, juce::AudioProcessorValueTreeState& apvts);
    ~StripComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

protected:
    MixerStrip& strip;
    juce::Label nameLabel;
    juce::Slider levelSlider;
    juce::ToggleButton muteButton, soloButton;
    juce::ImageComponent icon;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAtt, soloAtt;

    virtual void updateMeters() = 0;
    void setupKnob (juce::Slider& s, const juce::String& paramID, juce::AudioProcessorValueTreeState& apvts);
    void setSliderColours (juce::Slider& s, juce::Colour c);

    fxme::FxmeLookAndFeel fxmeLookAndFeel;
};

class AmbisonicStripComponent : public StripComponent
{
public:
    AmbisonicStripComponent (AmbisonicStrip& s, juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
protected:
    void updateMeters() override;
private:
    AmbisonicStrip& ambStrip;
    juce::Slider azSlider, elSlider, wSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> azAtt, elAtt, wAtt;
    VuMeterComponent meterL, meterR;
};

class MSStripComponent : public StripComponent
{
public:
    MSStripComponent (MSStrip& s, juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
protected:
    void updateMeters() override;
private:
    MSStrip& msStrip;
    juce::Slider panSlider;
    juce::Slider wSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wAtt;
    VuMeterComponent meterL, meterR;
};

class StereoStripComponent : public StripComponent
{
public:
    StereoStripComponent (StereoStrip& s, juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
protected:
    void updateMeters() override;
private:
    StereoStrip& stereoStrip;
    juce::Slider panSlider;
    juce::Slider wSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wAtt;
    VuMeterComponent meterL, meterR;
};

class MonoStripComponent : public StripComponent
{
public:
    MonoStripComponent (MonoStrip& s, juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
protected:
    void updateMeters() override;
private:
    MonoStrip& monoStrip;
    juce::Slider panSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAtt;
    VuMeterComponent meter;
};

class StereoReverbStripComponent : public StripComponent
{
public:
    StereoReverbStripComponent (StereoReverbStrip& s, juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
protected:
    void updateMeters() override;
private:
    StereoReverbStrip& reverbStrip;
    juce::Slider panSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAtt;
    VuMeterComponent meterL, meterR;
};

class MonoReverbStripComponent : public StripComponent
{
public:
    MonoReverbStripComponent (MonoReverbStrip& s, juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
protected:
    void updateMeters() override;
private:
    MonoReverbStrip& reverbStrip;
    juce::Slider panSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAtt;
    VuMeterComponent meter;
};

class MasterStripComponent : public StripComponent
{
public:
    MasterStripComponent (MasterStrip& s, juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
protected:
    void updateMeters() override;
private:
    MasterStrip& masterStrip;
    juce::Slider panSlider, wSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAtt, wAtt;
    VuMeterComponent meterL, meterR;
};

class MixerComponent : public juce::Component
{
public:
    MixerComponent (Mixer& mixer, Sampler& sampler, juce::AudioProcessorValueTreeState& apvts);
    ~MixerComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Mixer& mixer;
    juce::AudioProcessorValueTreeState& apvts;
    juce::TabbedComponent tabs;
    juce::TooltipWindow tooltipWindow {nullptr, 50};

    // Dynamic Levels Component
    class LevelsComponent : public juce::Component
    {
    public:
        LevelsComponent (Mixer& m, juce::AudioProcessorValueTreeState& apvts);
        void resized() override;
        
    private:
        Mixer& mixer;
        std::vector<std::unique_ptr<StripComponent>> strips;
    };
    
    LevelsComponent levelsComp;
    SamplerComponent samplerComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerComponent)
};