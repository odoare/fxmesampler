/*
  ==============================================================================

    Tube.cpp
    Created: 3 Feb 2026 6:36:27pm
    Author:  doare

  ==============================================================================
*/

#include "Tube.h"

Tube::Tube() {}

void Tube::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    x1.clear();
    y1.clear();
}

void Tube::setDrive (float drivedB) { drive = juce::Decibels::decibelsToGain (drivedB); }
void Tube::setBias (float b)        { bias = b; }
void Tube::setOutput (float gaindB) { outputGain = juce::Decibels::decibelsToGain (gaindB); }
void Tube::setOn (bool shouldBeOn)  { on = shouldBeOn; }
bool Tube::isOn() const             { return on; }

void Tube::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam = apvts.getRawParameterValue (prefix + "_Tube_On");
    driveParam = apvts.getRawParameterValue (prefix + "_Tube_Drive");
    biasParam = apvts.getRawParameterValue (prefix + "_Tube_Bias");
    outParam = apvts.getRawParameterValue (prefix + "_Tube_Out");
}

void Tube::checkParameters()
{
    if (onParam && *onParam != lastOn) { setOn (*onParam > 0.5f); lastOn = *onParam; }
    if (driveParam && *driveParam != lastDrive) { setDrive (*driveParam); lastDrive = *driveParam; }
    if (biasParam && *biasParam != lastBias) { setBias (*biasParam); lastBias = *biasParam; }
    if (outParam && *outParam != lastOut) { setOutput (*outParam); lastOut = *outParam; }
}

void Tube::process (juce::AudioBuffer<float>& buffer)
{
    if (!on) return;

    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    if ((int)x1.size() < numChannels)
    {
        x1.resize (numChannels, 0.0f);
        y1.resize (numChannels, 0.0f);
    }

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        float x_1 = x1[ch];
        float y_1 = y1[ch];

        for (int i = 0; i < numSamples; ++i)
        {
            float in = data[i] * drive;

            // Asymmetric Saturation: tanh(in + bias)
            // The bias shifts the signal on the tanh curve, creating even harmonics.
            float saturated = std::tanh (in + bias);

            // DC Blocker to remove the DC offset introduced by the bias
            // y[n] = x[n] - x[n-1] + R * y[n-1]
            float out = saturated - x_1 + 0.995f * y_1;

            x_1 = saturated;
            y_1 = out;

            data[i] = out * outputGain;
        }
        x1[ch] = x_1;
        y1[ch] = y_1;
    }
}
