/*
  ==============================================================================

    Compressor.cpp

  ==============================================================================
*/

#include "Compressor.h"

Compressor::Compressor()
{
    updateCoefficients();
}

void Compressor::prepare (double sampleRate, int numChannels)
{
    currentSampleRate = sampleRate;
    envelope = 0.0f;
    grMeter.prepare (sampleRate);
    updateCoefficients();
    juce::ignoreUnused (numChannels);
}

void Compressor::updateCoefficients()
{
    attackCoef = std::exp (-1.0f / (attackTimeMs * 0.001f * (float) currentSampleRate));
    releaseCoef = std::exp (-1.0f / (releaseTimeMs * 0.001f * (float) currentSampleRate));
    makeUpGain = juce::Decibels::decibelsToGain (postGaindB);
}

void Compressor::setAttack (float attackMs)
{
    attackTimeMs = attackMs;
    updateCoefficients();
}

void Compressor::setRelease (float releaseMs)
{
    releaseTimeMs = releaseMs;
    updateCoefficients();
}

void Compressor::setThreshold (float threshold)
{
    thresholddB = threshold;
}

void Compressor::setRatio (float r)
{
    ratio = r;
}

void Compressor::setPostGain (float gaindB)
{
    postGaindB = gaindB;
    updateCoefficients();
}

void Compressor::setOn (bool shouldBeOn)
{
    on = shouldBeOn;
}

bool Compressor::isOn() const
{
    return on;
}

void Compressor::process (juce::AudioBuffer<float>& buffer)
{
    if (! on)
        return;

    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    auto* channelData = buffer.getArrayOfWritePointers();

    for (int i = 0; i < numSamples; ++i)
    {
        // Sidechain: Max of all channels (Linked operation)
        float inputAbs = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float s = std::abs (channelData[ch][i]);
            if (s > inputAbs) inputAbs = s;
        }

        // Envelope Follower (Peak)
        if (inputAbs > envelope)
            envelope = attackCoef * envelope + (1.0f - attackCoef) * inputAbs;
        else
            envelope = releaseCoef * envelope + (1.0f - releaseCoef) * inputAbs;

        // Gain Computer
        float envdB = juce::Decibels::gainToDecibels (envelope);
        float gainReductiondB = 0.0f;

        if (envdB > thresholddB)
            gainReductiondB = (thresholddB - envdB) * (1.0f - 1.0f / ratio);

        float grLinear = juce::Decibels::decibelsToGain (gainReductiondB);
        grMeter.process (&grLinear, 1);

        float gain = juce::Decibels::decibelsToGain (gainReductiondB) * makeUpGain;

        // Apply Gain
        for (int ch = 0; ch < numChannels; ++ch)
        {
            channelData[ch][i] *= gain;
        }
    }
}