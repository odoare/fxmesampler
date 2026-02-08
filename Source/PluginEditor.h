/*
  ==============================================================================

    PluginEditor.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "MixerComponent.h"

/**
 * @class SimpleSamplerAudioProcessorEditor
 * @brief The editor (GUI) for the SimpleSampler plugin.
 */
class SimpleSamplerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    /**
     * @brief Constructor.
     * @param p The AudioProcessor to edit.
     */
    SimpleSamplerAudioProcessorEditor (SimpleSamplerAudioProcessor&);
    ~SimpleSamplerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SimpleSamplerAudioProcessor& audioProcessor;
    MixerComponent mixerComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleSamplerAudioProcessorEditor)
};