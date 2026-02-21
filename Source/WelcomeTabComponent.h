/*
  ==============================================================================

    WelcomeTabComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class WelcomeTabComponent : public juce::Component
{
public:
    WelcomeTabComponent(const juce::String& text, const juce::Image& image, juce::AudioProcessorValueTreeState& apvts);
    ~WelcomeTabComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::String text;
    juce::Image img;
    juce::AudioProcessorValueTreeState& apvts;
    juce::TextButton saveButton;
    juce::TextButton loadButton;
    std::unique_ptr<juce::FileChooser> fc;

    void savePreset();
    void loadPreset();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WelcomeTabComponent)
};