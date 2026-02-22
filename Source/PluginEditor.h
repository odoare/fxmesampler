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
 * @class FxmeSamplerAudioProcessorEditor
 * @brief The editor (GUI) for the FxmeSampler plugin.
 */
class FxmeSamplerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    /**
     * @brief Constructor.
     * @param p The AudioProcessor to edit.
     */
    FxmeSamplerAudioProcessorEditor (FxmeSamplerAudioProcessor&);
    ~FxmeSamplerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FxmeSamplerAudioProcessor& audioProcessor;
    MixerComponent mixerComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxmeSamplerAudioProcessorEditor)
};