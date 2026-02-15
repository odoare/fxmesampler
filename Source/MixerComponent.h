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
#include "EffectChainReverbComponent.h"
#include "ConvolReverbComponent.h" // For direct strip reverb IR dropdown
#include "Knob.h"

//==============================================================================
/**
 * @class StripComponent
 * @brief Base class for a mixer strip GUI component.
 */
class StripComponent : public juce::Component, public juce::Timer
{
public:
    /**
     * @brief Constructor.
     * @param strip The MixerStrip to control.
     * @param apvts The APVTS for parameter attachments.
     */
    StripComponent (MixerStrip& strip, juce::AudioProcessorValueTreeState& apvts);
    ~StripComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    /**
     * @brief Timer callback for updating meters.
     */
    void timerCallback() override;
    
    void setStripColor(juce::Colour c);

protected:
    MixerStrip& strip;
    juce::Label nameLabel;
    juce::Slider levelSlider;
    juce::ToggleButton muteButton, soloButton;
    juce::ImageComponent icon;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAtt, soloAtt;
    std::vector<std::unique_ptr<juce::Slider>> sendSliders;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sendAtts;

    std::vector<std::unique_ptr<juce::ToggleButton>> prePostButtons;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> prePostAtts;

    std::vector<std::unique_ptr<juce::ToggleButton>> routeButtons;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> routeAtts;
    void createRouteButtons(juce::AudioProcessorValueTreeState& apvts);

    virtual void updateMeters() = 0;
    void setupKnob (juce::Slider& s, const juce::String& paramID, juce::AudioProcessorValueTreeState& apvts);
    void setSliderColours (juce::Slider& s, juce::Colour c);

    fxme::FxmeLookAndFeel fxmeLookAndFeel;
};

/**
 * @class AmbisonicStripComponent
 * @brief GUI component for an Ambisonic mixer strip.
 */
class AmbisonicStripComponent : public StripComponent
{
public:
    /**
     * @brief Constructor.
     * @param s The AmbisonicStrip.
     * @param apvts The APVTS.
     */
    AmbisonicStripComponent (AmbisonicStrip& s, juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
protected:
    void updateMeters() override;
private:
    AmbisonicStrip& ambStrip;
    Knob azSlider, elSlider, wSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> azAtt, elAtt, wAtt;
    VuMeterComponent meterL, meterR;
};

/**
 * @class MSStripComponent
 * @brief GUI component for a Mid-Side mixer strip.
 */
class MSStripComponent : public StripComponent
{
public:
    /**
     * @brief Constructor.
     * @param s The MSStrip.
     * @param apvts The APVTS.
     */
    MSStripComponent (MSStrip& s, juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
protected:
    void updateMeters() override;
private:
    MSStrip& msStrip;
    Knob panSlider;
    Knob wSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wAtt;
    VuMeterComponent meterL, meterR;
};

/**
 * @class StereoStripComponent
 * @brief GUI component for a Stereo mixer strip.
 */
class StereoStripComponent : public StripComponent
{
public:
    /**
     * @brief Constructor.
     * @param s The StereoStrip.
     * @param apvts The APVTS.
     */
    StereoStripComponent (StereoStrip& s, juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
protected:
    void updateMeters() override;
private:
    StereoStrip& stereoStrip;
    Knob panSlider;
    Knob wSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wAtt;
    VuMeterComponent meterL, meterR;
};

/**
 * @class MonoStripComponent
 * @brief GUI component for a Mono mixer strip.
 */
class MonoStripComponent : public StripComponent
{
public:
    /**
     * @brief Constructor.
     * @param s The MonoStrip.
     * @param apvts The APVTS.
     */
    MonoStripComponent (MonoStrip& s, juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
protected:
    void updateMeters() override;
private:
    MonoStrip& monoStrip;
    Knob panSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAtt;
    VuMeterComponent meter;
};

/**
 * @class StereoReverbStripComponent
 * @brief GUI component for a Stereo Reverb mixer strip.
 */
class StereoReverbStripComponent : public StripComponent
{
public:
    /**
     * @brief Constructor.
     * @param s The StereoReverbStrip.
     * @param apvts The APVTS.
     */
    StereoReverbStripComponent (StereoReverbStrip& s, juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
protected:
    void updateMeters() override;
private:
    StereoReverbStrip& reverbStrip;
    Knob panSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAtt;
    juce::ComboBox irBox; // For IR selection dropdown
    juce::Label irLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> irAtt;
    VuMeterComponent meterL, meterR;
};

/**
 * @class MonoReverbStripComponent
 * @brief GUI component for a Mono Reverb mixer strip.
 */
class MonoReverbStripComponent : public StripComponent
{
public:
    /**
     * @brief Constructor.
     * @param s The MonoReverbStrip.
     * @param apvts The APVTS.
     */
    MonoReverbStripComponent (MonoReverbStrip& s, juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
protected:
    void updateMeters() override;
private:
    MonoReverbStrip& reverbStrip;
    Knob panSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAtt;
    juce::ComboBox irBox; // For IR selection dropdown
    juce::Label irLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> irAtt;
    VuMeterComponent meter;
};

/**
 * @class BusStripComponent
 * @brief GUI component for a Bus mixer strip.
 */
class BusStripComponent : public StripComponent
{
public:
    BusStripComponent (BusStrip& s, juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
protected:
    void updateMeters() override;
private:
    BusStrip& busStrip;
    Knob panSlider, wSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAtt, wAtt;
    VuMeterComponent meterL, meterR;
};

/**
 * @class MasterStripComponent
 * @brief GUI component for the Master mixer strip.
 */
class MasterStripComponent : public StripComponent
{
public:
    /**
     * @brief Constructor.
     * @param s The MasterStrip.
     * @param apvts The APVTS.
     */
    MasterStripComponent (MasterStrip& s, juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
protected:
    void updateMeters() override;
private:
    MasterStrip& masterStrip;
    Knob panSlider, wSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAtt, wAtt;
    VuMeterComponent meterL, meterR;
};

class WelcomeComponent : public juce::Component
{
public:
    WelcomeComponent() = default;
    void setText(const juce::String& t) { text = t; }
    void setImage(const juce::Image& i) { img = i; }
    void paint(juce::Graphics& g) override;
    void resized() override;
private:
    juce::String text;
    juce::Image img;
};

/**
 * @class MixerComponent
 * @brief The main component containing the mixer interface.
 */
class MixerComponent : public juce::Component
{
public:
    /**
     * @brief Constructor.
     * @param mixer The Mixer instance.
     * @param sampler The Sampler instance.
     * @param apvts The APVTS.
     */
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
    WelcomeComponent welcomeComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerComponent)
};