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
    envelope = 0.0f;
    x1.clear();
    y1.clear();
}

void Tube::setDrive (float drivedB) { drive = juce::Decibels::decibelsToGain (drivedB); }
void Tube::setBias (float b)        { bias = b; }
void Tube::setOutput (float gaindB) { outputGain = juce::Decibels::decibelsToGain (gaindB); }
void Tube::setOn (bool shouldBeOn)  { on = shouldBeOn; }
void Tube::setModel (TubeModel m)   { currentModel = m; }
bool Tube::isOn() const             { return on; }

void Tube::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam = apvts.getRawParameterValue (prefix + "_Tube_On");
    driveParam = apvts.getRawParameterValue (prefix + "_Tube_Drive");
    biasParam = apvts.getRawParameterValue (prefix + "_Tube_Bias");
    outParam = apvts.getRawParameterValue (prefix + "_Tube_Out");
    modelParam = apvts.getRawParameterValue (prefix + "_Tube_Model");
}

void Tube::checkParameters()
{
    if (onParam && *onParam != lastOn) { setOn (*onParam > 0.5f); lastOn = *onParam; }
    if (driveParam && *driveParam != lastDrive) { setDrive (*driveParam); lastDrive = *driveParam; }
    if (biasParam && *biasParam != lastBias) { setBias (*biasParam); lastBias = *biasParam; }
    if (outParam && *outParam != lastOut) { setOutput (*outParam); lastOut = *outParam; }
    if (modelParam && *modelParam != lastModel) { setModel (static_cast<TubeModel>((int)*modelParam)); lastModel = *modelParam; }
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

    // Coefficients for envelope follower (approx 50ms attack, 100ms release)
    // Slower attack reduces control signal feedthrough (thump)
    const float attackCoef = std::exp (-1.0f / (0.07f * (float)currentSampleRate));
    const float releaseCoef = std::exp (-1.0f / (0.10f * (float)currentSampleRate));

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        float x_1 = x1[ch];
        float y_1 = y1[ch];

        for (int i = 0; i < numSamples; ++i)
        {
            float in = data[i] * drive;

            float effectiveBias = bias;

            if (currentModel == TubeModel::Dynamic)
            {
                float absIn = std::abs (in);
                if (absIn > envelope)
                    envelope = attackCoef * envelope + (1.0f - attackCoef) * absIn;
                else
                    envelope = releaseCoef * envelope + (1.0f - releaseCoef) * absIn;
                
                // Sag: Reduce bias (shift negative) as signal increases
                // Scaling factor 0.5 is arbitrary but effective
                effectiveBias -= envelope * 0.5f;
            }

            // Asymmetric Saturation: tanh(in + bias)
            // The bias shifts the signal on the tanh curve, creating even harmonics.
            float saturated = std::tanh (in + effectiveBias);

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
