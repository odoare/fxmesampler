/*
  ==============================================================================

    Tube.h
    Created: 3 Feb 2026 6:36:27pm
    Author:  doare

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>

class Tube
{
public:
    Tube();

    void prepare (double sampleRate);
    void process (juce::AudioBuffer<float>& buffer);

    void setDrive (float drivedB);
    void setBias (float bias);
    void setOutput (float gaindB);
    void setOn (bool shouldBeOn);
    bool isOn() const;

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();

private:
    double currentSampleRate = 44100.0;
    bool on = true;

    float drive = 1.0f;
    float bias = 0.0f;
    float outputGain = 1.0f;

    std::vector<float> x1, y1; // DC Blocker state

    std::atomic<float>* onParam = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* biasParam = nullptr;
    std::atomic<float>* outParam = nullptr;

    float lastOn = -1.0f, lastDrive = -100.0f, lastBias = -1.0f, lastOut = -100.0f;
};