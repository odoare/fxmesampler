/*
  ==============================================================================

    AmbixToMS.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class AmbixToMS
{
public:
    AmbixToMS() = default;

    void setAzimuth (float degrees);
    void setElevation (float degrees);
    void setWidth (float width);
    void setLevel (float level);

    void process (const juce::AudioBuffer<float>& inputBuffer, juce::AudioBuffer<float>& outputBuffer);

private:
    float azimuth = 0.0f;
    float elevation = 0.0f;
    float width = 1.0f;
    float level = 1.0f;
};