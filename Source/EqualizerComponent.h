/*
  ==============================================================================

    EqualizerComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Equalizer.h"

class FrequencyResponseGraph : public juce::Component
{
public:
    FrequencyResponseGraph() = default;
    void setReferences (juce::Slider& lsF, juce::Slider& lsG,
                        juce::Slider& b1F, juce::Slider& b1Q, juce::Slider& b1G,
                        juce::Slider& b2F, juce::Slider& b2Q, juce::Slider& b2G,
                        juce::Slider& hsF, juce::Slider& hsG,
                        juce::Slider& preG, juce::Slider& postG,
                        juce::ToggleButton& onB);
    void paint (juce::Graphics& g) override;
    void updateCurve();

private:
    juce::Slider *lsFreq = nullptr, *lsGain = nullptr;
    juce::Slider *b1Freq = nullptr, *b1Q = nullptr, *b1Gain = nullptr;
    juce::Slider *b2Freq = nullptr, *b2Q = nullptr, *b2Gain = nullptr;
    juce::Slider *hsFreq = nullptr, *hsGain = nullptr;
    juce::Slider *preGain = nullptr, *postGain = nullptr;
    juce::ToggleButton* onBtn = nullptr;

    juce::Path curvePath;
};

class EqualizerComponent : public juce::Component
{
public:
    EqualizerComponent (Equalizer& equalizerToControl, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~EqualizerComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Equalizer& equalizer;
    juce::AudioProcessorValueTreeState& apvts;

    juce::ToggleButton onButton;
    juce::Label titleLabel;
    
    // LowShelf, Band1, Band2, HighShelf
    // LS: F, G. B1: F, Q, G. B2: F, Q, G. HS: F, G.
    juce::Slider lsFreq, lsGain;
    juce::Slider b1Freq, b1Q, b1Gain;
    juce::Slider b2Freq, b2Q, b2Gain;
    juce::Slider hsFreq, hsGain;
    juce::Slider preGainSlider, postGainSlider;

    FrequencyResponseGraph responseGraph;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ButtonAttachment> onAtt;
    std::unique_ptr<SliderAttachment> preGainAtt;
    std::unique_ptr<SliderAttachment> postGainAtt;
    std::unique_ptr<SliderAttachment> lsFreqAtt, lsGainAtt;
    std::unique_ptr<SliderAttachment> b1FreqAtt, b1QAtt, b1GainAtt;
    std::unique_ptr<SliderAttachment> b2FreqAtt, b2QAtt, b2GainAtt;
    std::unique_ptr<SliderAttachment> hsFreqAtt, hsGainAtt;

    void setSliderColours (juce::Slider& s, juce::Colour c);
    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqualizerComponent)
};