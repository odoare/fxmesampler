/*
  ==============================================================================

    Compressor.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class Compressor
{
public:
    Compressor();

    void prepare (double sampleRate, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    void setAttack (float attackMs);
    void setRelease (float releaseMs); // Corresponds to "decay time" in request
    void setThreshold (float thresholddB);
    void setRatio (float ratio);
    void setPostGain (float gaindB);
    void setOn (bool shouldBeOn);
    bool isOn() const;

private:
    double currentSampleRate = 44100.0;

    float attackTimeMs = 10.0f;
    float releaseTimeMs = 100.0f;
    float thresholddB = 0.0f;
    float ratio = 1.0f;
    float postGaindB = 0.0f;
    bool on = true;

    float envelope = 0.0f;

    // Coefficients
    float attackCoef = 0.0f;
    float releaseCoef = 0.0f;
    float makeUpGain = 1.0f;

    void updateCoefficients();
};