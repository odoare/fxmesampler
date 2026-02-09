/*
  ==============================================================================

    EffectChainDynamics.cpp

  ==============================================================================
*/

#include "EffectChainDynamics.h"

EffectChainDynamics::EffectChainDynamics()
{
}

void EffectChainDynamics::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    eq.prepare (sampleRate, numChannels);
    comp.prepare (sampleRate, numChannels);
    tube.prepare (sampleRate);
    juce::ignoreUnused (samplesPerBlock);
}

void EffectChainDynamics::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    orderParam = apvts.getRawParameterValue (prefix + "_Order");
    eq.assignParameters (apvts, prefix);
    comp.assignParameters (apvts, prefix);
    tube.assignParameters (apvts, prefix);
}

void EffectChainDynamics::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix)
{
    juce::StringArray orderOptions;
    orderOptions.add ("EQ -> Comp -> Tube");
    orderOptions.add ("EQ -> Tube -> Comp");
    orderOptions.add ("Comp -> EQ -> Tube");
    orderOptions.add ("Comp -> Tube -> EQ");
    orderOptions.add ("Tube -> EQ -> Comp");
    orderOptions.add ("Tube -> Comp -> EQ");
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { prefix + "_Order", 1 }, prefix + " Order", orderOptions, 0));

    Equalizer::addParameters (params, prefix);
    Compressor::addParameters (params, prefix);
    Tube::addParameters (params, prefix);
}

void EffectChainDynamics::checkParameters()
{
    eq.checkParameters();
    comp.checkParameters();
    tube.checkParameters();
}

void EffectChainDynamics::process (juce::AudioBuffer<float>& buffer)
{
    checkParameters();

    int order = 0;
    if (orderParam) order = (int) *orderParam;

    // 0: EQ -> Comp -> Tube
    // 1: EQ -> Tube -> Comp
    // 2: Comp -> EQ -> Tube
    // 3: Comp -> Tube -> EQ
    // 4: Tube -> EQ -> Comp
    // 5: Tube -> Comp -> EQ

    switch (order)
    {
        case 0: eq.process (buffer); comp.process (buffer); tube.process (buffer); break;
        case 1: eq.process (buffer); tube.process (buffer); comp.process (buffer); break;
        case 2: comp.process (buffer); eq.process (buffer); tube.process (buffer); break;
        case 3: comp.process (buffer); tube.process (buffer); eq.process (buffer); break;
        case 4: tube.process (buffer); eq.process (buffer); comp.process (buffer); break;
        case 5: tube.process (buffer); comp.process (buffer); eq.process (buffer); break;
        default: eq.process (buffer); comp.process (buffer); tube.process (buffer); break;
    }
}
