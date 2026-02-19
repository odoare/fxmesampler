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
    WelcomeTabComponent(juce::Component& content, juce::AudioProcessorValueTreeState& apvts);
    ~WelcomeTabComponent() override;

    void resized() override;

private:
    juce::Component& content;
    juce::AudioProcessorValueTreeState& apvts;
    juce::TextButton saveButton;
    juce::TextButton loadButton;
    std::unique_ptr<juce::FileChooser> fc;

    void savePreset();
    void loadPreset();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WelcomeTabComponent)
};