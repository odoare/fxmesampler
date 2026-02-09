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

void Compressor::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    onParam = apvts.getRawParameterValue (prefix + "_Comp_On");
    preGainParam = apvts.getRawParameterValue (prefix + "_Comp_PreGain");
    attackParam = apvts.getRawParameterValue (prefix + "_Comp_Attack");
    releaseParam = apvts.getRawParameterValue (prefix + "_Comp_Release");
    threshParam = apvts.getRawParameterValue (prefix + "_Comp_Thresh");
    ratioParam = apvts.getRawParameterValue (prefix + "_Comp_Ratio");
    gainParam = apvts.getRawParameterValue (prefix + "_Comp_Gain");
}

void Compressor::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix)
{
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { prefix + "_Comp_On", 1 }, prefix + " Comp On", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Comp_PreGain", 1 }, prefix + " Comp Pre Gain", -24.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Comp_Attack", 1 }, prefix + " Comp Attack", 0.1f, 100.0f, 10.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Comp_Release", 1 }, prefix + " Comp Release", 10.0f, 1000.0f, 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Comp_Thresh", 1 }, prefix + " Comp Thresh", -60.0f, 0.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Comp_Ratio", 1 }, prefix + " Comp Ratio", 1.0f, 20.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Comp_Gain", 1 }, prefix + " Comp Gain", -24.0f, 24.0f, 0.0f));
}

void Compressor::checkParameters()
{
    if (onParam && *onParam != lastOn) { setOn (*onParam > 0.5f); lastOn = *onParam; }

    if (preGainParam && *preGainParam != lastPreGain) { preGain = juce::Decibels::decibelsToGain (preGainParam->load()); lastPreGain = *preGainParam; }
    
    bool changed = false;
    if (attackParam && *attackParam != lastAttack) { attackTimeMs = *attackParam; lastAttack = attackTimeMs; changed = true; }
    if (releaseParam && *releaseParam != lastRelease) { releaseTimeMs = *releaseParam; lastRelease = releaseTimeMs; changed = true; }
    if (gainParam && *gainParam != lastGain) { postGaindB = *gainParam; lastGain = postGaindB; changed = true; }
    
    if (changed)
        updateCoefficients();

    // These don't require coefficient update
    if (threshParam) thresholddB = *threshParam;
    if (ratioParam) ratio = *ratioParam;
}

void Compressor::process (juce::AudioBuffer<float>& buffer)
{
    if (! on)
        return;

    buffer.applyGain (preGain);

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