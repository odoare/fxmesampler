/*
  ==============================================================================

    MixerStrips.cpp

  ==============================================================================
*/

#include "MixerStrips.h"
#include "EffectChainDelay.h"
#include "EffectChainDynamics.h"
#include "EffectChainReverb.h"

//==============================================================================
void MixerStrip::processEffects (juce::AudioBuffer<float>& buffer)
{
    if (effectChain)
        effectChain->process (buffer);
}

void MixerStrip::addSend (const juce::String& busName, BusStrip* bus)
{
    Send s;
    s.busName = busName;
    s.bus = bus;
    sends.push_back (s);
}

void MixerStrip::setBPM(double bpm)
{
    if (effectChain)
        effectChain->setBPM(bpm);
}

void MixerStrip::processSends (juce::AudioBuffer<float>& buffer, bool isPre)
{
    for (auto& send : sends)
    {
        bool sendIsPre = (send.prePostParam && *send.prePostParam > 0.5f);
        if (sendIsPre == isPre)
        {
            if (send.gainParam) send.currentGain = juce::Decibels::decibelsToGain (send.gainParam->load());
            if (send.bus && send.currentGain > 0.0001f)
                send.bus->addInput (buffer, send.currentGain);
        }
    }
}

//==============================================================================
AmbisonicStrip::AmbisonicStrip (const juce::String& n) : MixerStrip (n) {}

void AmbisonicStrip::prepare (double sampleRate, int samplesPerBlock)
{
    if (!effectChain) effectChain = std::make_unique<EffectChainDynamics>();
    effectChain->prepare (sampleRate, samplesPerBlock, 2);

    meterL.prepare (sampleRate);
    meterR.prepare (sampleRate);
    tempBuffer.setSize (2, samplesPerBlock);
}

void AmbisonicStrip::assignParameters (juce::AudioProcessorValueTreeState& apvts)
{
    azParam = apvts.getRawParameterValue (name + "_Azimuth");
    elParam = apvts.getRawParameterValue (name + "_Elevation");
    wParam = apvts.getRawParameterValue (name + "_Width");
    lvlParam = apvts.getRawParameterValue (name + "_Level");
    muteParam = apvts.getRawParameterValue (name + "_Mute");
    soloParam = apvts.getRawParameterValue (name + "_Solo");
    
    if (effectChain) effectChain->assignParameters (apvts, name);

    for (auto& send : sends)
    {
        juce::String paramID = name + "_Send_" + send.busName;
        send.gainParam = apvts.getRawParameterValue (paramID);
        juce::String preParamID = name + "_Send_" + send.busName + "_Pre";
        send.prePostParam = apvts.getRawParameterValue (preParamID);
    }

    routeParams.resize(4);
    routeParams[0] = apvts.getRawParameterValue(name + "_Route_Main");
    routeParams[1] = apvts.getRawParameterValue(name + "_Route_Aux1");
    routeParams[2] = apvts.getRawParameterValue(name + "_Route_Aux2");
    routeParams[3] = apvts.getRawParameterValue(name + "_Route_Aux3");
}

void AmbisonicStrip::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Azimuth", 1 }, name + " Azimuth", -180.0f, 180.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Elevation", 1 }, name + " Elevation", -90.0f, 90.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Width", 1 }, name + " Width", 0.0f, 2.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", -60.0f, 6.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Mute", 1 }, name + " Mute", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Solo", 1 }, name + " Solo", false));

    if (effectChain) effectChain->addParameters (params, name);

    for (auto& send : sends)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Send_" + send.busName, 1 }, name + " Send " + send.busName, -60.0f, 6.0f, -60.0f));
        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Send_" + send.busName + "_Pre", 1 }, name + " Send " + send.busName + " Pre", false));
    }

    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Main", 1 }, name + " Route Main", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux1", 1 }, name + " Route Aux 1", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux2", 1 }, name + " Route Aux 2", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux3", 1 }, name + " Route Aux 3", false));
}

void AmbisonicStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset)
{
    if (azParam) ambix.setAzimuth (*azParam);
    if (elParam) ambix.setElevation (*elParam);
    if (wParam) ambix.setWidth (*wParam);
    
    float currentLevel = 1.0f;
    if (lvlParam) currentLevel = juce::Decibels::decibelsToGain (lvlParam->load());
    ambix.setLevel(1.0f); // Decode at unity to allow pre-fader sends

    if (tempBuffer.getNumSamples() != input.getNumSamples())
        tempBuffer.setSize (2, input.getNumSamples(), false, false, true);

    tempBuffer.clear();
    // AmbixToMS expects 4 channels starting at 0 in its input buffer logic, 
    // but we pass a subset or use pointers. AmbixToMS::process takes buffers.
    // const_cast is ugly but AudioBuffer doesn't have a const-correct subset creation easily without copying
    // However, AmbixToMS::process takes const input.
    // We can create a buffer that points to the data.
    const float* readPointers[4];
    for (int i = 0; i < 4; ++i)
        readPointers[i] = input.getReadPointer (inputChannelOffset + i);
    
    juce::AudioBuffer<float> inputSubset (const_cast<float**>(readPointers), 4, input.getNumSamples());

    ambix.process (inputSubset, tempBuffer); // Adds to tempBuffer
    
    processSends (tempBuffer, true); // Pre FX+Fader
    processEffects (tempBuffer);
    
    tempBuffer.applyGain (currentLevel);
    processSends (tempBuffer, false); // Post FX+Fader

    meterL.process (tempBuffer.getReadPointer (0), tempBuffer.getNumSamples());
    meterR.process (tempBuffer.getReadPointer (1), tempBuffer.getNumSamples());

    // Routing
    if (routeParams[0] && *routeParams[0] > 0.5f)
        for (int ch = 0; ch < 2; ++ch)
            mixBuffer.addFrom (ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());

    if (routeParams[1] && *routeParams[1] > 0.5f && outputBuffer.getNumChannels() >= 4)
        for (int ch = 0; ch < 2; ++ch)
            outputBuffer.addFrom (2 + ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());

    if (routeParams[2] && *routeParams[2] > 0.5f && outputBuffer.getNumChannels() >= 6)
        for (int ch = 0; ch < 2; ++ch)
            outputBuffer.addFrom (4 + ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());

    if (routeParams[3] && *routeParams[3] > 0.5f && outputBuffer.getNumChannels() >= 8)
        for (int ch = 0; ch < 2; ++ch)
            outputBuffer.addFrom (6 + ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());
}

void AmbisonicStrip::clearMeters()
{
    meterL.clear();
    meterR.clear();
}

//==============================================================================
MSStrip::MSStrip (const juce::String& n) : MixerStrip (n) {}

void MSStrip::prepare (double sampleRate, int samplesPerBlock)
{
    if (!effectChain) effectChain = std::make_unique<EffectChainDynamics>();
    effectChain->prepare (sampleRate, samplesPerBlock, 2);

    meterL.prepare (sampleRate);
    meterR.prepare (sampleRate);
    tempBuffer.setSize (2, samplesPerBlock);
}

void MSStrip::assignParameters (juce::AudioProcessorValueTreeState& apvts)
{
    panParam = apvts.getRawParameterValue (name + "_Pan");
    wParam = apvts.getRawParameterValue (name + "_Width");
    lvlParam = apvts.getRawParameterValue (name + "_Level");
    muteParam = apvts.getRawParameterValue (name + "_Mute");
    soloParam = apvts.getRawParameterValue (name + "_Solo");
    
    if (effectChain) effectChain->assignParameters (apvts, name);

    for (auto& send : sends)
    {
        juce::String paramID = name + "_Send_" + send.busName;
        send.gainParam = apvts.getRawParameterValue (paramID);
        juce::String preParamID = name + "_Send_" + send.busName + "_Pre";
        send.prePostParam = apvts.getRawParameterValue (preParamID);
    }

    routeParams.resize(4);
    routeParams[0] = apvts.getRawParameterValue(name + "_Route_Main");
    routeParams[1] = apvts.getRawParameterValue(name + "_Route_Aux1");
    routeParams[2] = apvts.getRawParameterValue(name + "_Route_Aux2");
    routeParams[3] = apvts.getRawParameterValue(name + "_Route_Aux3");
}

void MSStrip::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Pan", 1 }, name + " Pan", -1.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Width", 1 }, name + " Width", 0.0f, 2.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", -60.0f, 6.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Mute", 1 }, name + " Mute", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Solo", 1 }, name + " Solo", false));

    if (effectChain) effectChain->addParameters (params, name);

    for (auto& send : sends)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Send_" + send.busName, 1 }, name + " Send " + send.busName, -60.0f, 6.0f, -60.0f));
        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Send_" + send.busName + "_Pre", 1 }, name + " Send " + send.busName + " Pre", false));
    }

    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Main", 1 }, name + " Route Main", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux1", 1 }, name + " Route Aux 1", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux2", 1 }, name + " Route Aux 2", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux3", 1 }, name + " Route Aux 3", false));
}

void MSStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset)
{
    if (panParam) pan = *panParam;
    if (wParam) width = *wParam;
    if (lvlParam) level = juce::Decibels::decibelsToGain (lvlParam->load());

    if (tempBuffer.getNumSamples() != input.getNumSamples())
        tempBuffer.setSize (2, input.getNumSamples(), false, false, true);

    tempBuffer.clear();
    // Copy input to temp
    for (int i = 0; i < 2; ++i)
        tempBuffer.copyFrom (i, 0, input, inputChannelOffset + i, 0, input.getNumSamples());

    // Width processing and MS Decoding
    auto* l = tempBuffer.getWritePointer(0);
    auto* r = tempBuffer.getWritePointer(1);
    for (int i = 0; i < tempBuffer.getNumSamples(); ++i)
    {
        float mid = l[i];
        float side = r[i] * width;
        l[i] = mid + side;
        r[i] = mid - side;
    }

    processSends (tempBuffer, true); // Pre FX+Fader
    processEffects (tempBuffer);
    
    tempBuffer.applyGain (level);
    processSends (tempBuffer, false); // Post FX+Fader

    // Balance using constant-power law
    float panRad = (1.0f + pan) * juce::MathConstants<float>::halfPi * 0.5f;
    float gainL = juce::dsp::FastMathApproximations::cos (panRad);
    float gainR = juce::dsp::FastMathApproximations::sin (panRad);
    tempBuffer.applyGain (0, 0, tempBuffer.getNumSamples(), gainL);
    tempBuffer.applyGain (1, 0, tempBuffer.getNumSamples(), gainR);

    meterL.process (tempBuffer.getReadPointer (0), tempBuffer.getNumSamples());
    meterR.process (tempBuffer.getReadPointer (1), tempBuffer.getNumSamples());

    // Routing
    if (routeParams[0] && *routeParams[0] > 0.5f)
        for (int ch = 0; ch < 2; ++ch)
            mixBuffer.addFrom (ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());

    if (routeParams[1] && *routeParams[1] > 0.5f && outputBuffer.getNumChannels() >= 4)
        for (int ch = 0; ch < 2; ++ch)
            outputBuffer.addFrom (2 + ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());

    if (routeParams[2] && *routeParams[2] > 0.5f && outputBuffer.getNumChannels() >= 6)
        for (int ch = 0; ch < 2; ++ch)
            outputBuffer.addFrom (4 + ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());

    if (routeParams[3] && *routeParams[3] > 0.5f && outputBuffer.getNumChannels() >= 8)
        for (int ch = 0; ch < 2; ++ch)
            outputBuffer.addFrom (6 + ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());
}

void MSStrip::clearMeters()
{
    meterL.clear();
    meterR.clear();
}

//==============================================================================
StereoStrip::StereoStrip (const juce::String& n) : MixerStrip (n) {}

void StereoStrip::prepare (double sampleRate, int samplesPerBlock)
{
    if (!effectChain) effectChain = std::make_unique<EffectChainDynamics>();
    effectChain->prepare (sampleRate, samplesPerBlock, 2);

    meterL.prepare (sampleRate);
    meterR.prepare (sampleRate);
    tempBuffer.setSize (2, samplesPerBlock);
}

void StereoStrip::assignParameters (juce::AudioProcessorValueTreeState& apvts)
{
    panParam = apvts.getRawParameterValue (name + "_Pan");
    wParam = apvts.getRawParameterValue (name + "_Width");
    lvlParam = apvts.getRawParameterValue (name + "_Level");
    muteParam = apvts.getRawParameterValue (name + "_Mute");
    soloParam = apvts.getRawParameterValue (name + "_Solo");
    
    if (effectChain) effectChain->assignParameters (apvts, name);

    for (auto& send : sends)
    {
        juce::String paramID = name + "_Send_" + send.busName;
        send.gainParam = apvts.getRawParameterValue (paramID);
        juce::String preParamID = name + "_Send_" + send.busName + "_Pre";
        send.prePostParam = apvts.getRawParameterValue (preParamID);
    }

    routeParams.resize(4);
    routeParams[0] = apvts.getRawParameterValue(name + "_Route_Main");
    routeParams[1] = apvts.getRawParameterValue(name + "_Route_Aux1");
    routeParams[2] = apvts.getRawParameterValue(name + "_Route_Aux2");
    routeParams[3] = apvts.getRawParameterValue(name + "_Route_Aux3");
}

void StereoStrip::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Pan", 1 }, name + " Pan", -1.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Width", 1 }, name + " Width", 0.0f, 2.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", -60.0f, 6.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Mute", 1 }, name + " Mute", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Solo", 1 }, name + " Solo", false));

    if (effectChain) effectChain->addParameters (params, name);

    for (auto& send : sends)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Send_" + send.busName, 1 }, name + " Send " + send.busName, -60.0f, 6.0f, -60.0f));
        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Send_" + send.busName + "_Pre", 1 }, name + " Send " + send.busName + " Pre", false));
    }

    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Main", 1 }, name + " Route Main", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux1", 1 }, name + " Route Aux 1", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux2", 1 }, name + " Route Aux 2", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux3", 1 }, name + " Route Aux 3", false));
}

void StereoStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset)
{
    if (panParam) pan = *panParam;
    if (wParam) width = *wParam;
    if (lvlParam) level = juce::Decibels::decibelsToGain (lvlParam->load());

    if (tempBuffer.getNumSamples() != input.getNumSamples())
        tempBuffer.setSize (2, input.getNumSamples(), false, false, true);

    tempBuffer.clear();
    // Copy input to temp
    for (int i = 0; i < 2; ++i)
        tempBuffer.copyFrom (i, 0, input, inputChannelOffset + i, 0, input.getNumSamples());

    // Width processing (Standard M/S width control for stereo sources)
    if (std::abs(width - 1.0f) > 0.001f)
    {
        auto* l = tempBuffer.getWritePointer(0);
        auto* r = tempBuffer.getWritePointer(1);
        for (int i = 0; i < tempBuffer.getNumSamples(); ++i)
        {
            float mid = (l[i] + r[i]) * 0.5f;
            float side = (l[i] - r[i]) * 0.5f * width;
            l[i] = mid + side;
            r[i] = mid - side;
        }
    }

    processSends (tempBuffer, true); // Pre FX+Fader
    processEffects (tempBuffer);
    
    tempBuffer.applyGain (level);
    processSends (tempBuffer, false); // Post FX+Fader

    // Balance using Linear law (Classical Stereo)
    float gainL = 0.5f * (1.0f - pan);
    float gainR = 0.5f * (1.0f + pan);
    
    tempBuffer.applyGain (0, 0, tempBuffer.getNumSamples(), gainL);
    tempBuffer.applyGain (1, 0, tempBuffer.getNumSamples(), gainR);

    meterL.process (tempBuffer.getReadPointer (0), tempBuffer.getNumSamples());
    meterR.process (tempBuffer.getReadPointer (1), tempBuffer.getNumSamples());

    // Routing
    if (routeParams[0] && *routeParams[0] > 0.5f)
        for (int ch = 0; ch < 2; ++ch)
            mixBuffer.addFrom (ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());

    if (routeParams[1] && *routeParams[1] > 0.5f && outputBuffer.getNumChannels() >= 4)
        for (int ch = 0; ch < 2; ++ch)
            outputBuffer.addFrom (2 + ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());

    if (routeParams[2] && *routeParams[2] > 0.5f && outputBuffer.getNumChannels() >= 6)
        for (int ch = 0; ch < 2; ++ch)
            outputBuffer.addFrom (4 + ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());

    if (routeParams[3] && *routeParams[3] > 0.5f && outputBuffer.getNumChannels() >= 8)
        for (int ch = 0; ch < 2; ++ch)
            outputBuffer.addFrom (6 + ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());
}

void StereoStrip::clearMeters()
{
    meterL.clear();
    meterR.clear();
}

//==============================================================================
MonoStrip::MonoStrip (const juce::String& n) : MixerStrip (n) {}

void MonoStrip::prepare (double sampleRate, int samplesPerBlock)
{
    if (!effectChain) effectChain = std::make_unique<EffectChainDynamics>();
    effectChain->prepare (sampleRate, samplesPerBlock, 1);

    meter.prepare (sampleRate);
    tempBuffer.setSize (1, samplesPerBlock);
}

void MonoStrip::assignParameters (juce::AudioProcessorValueTreeState& apvts)
{
    panParam = apvts.getRawParameterValue (name + "_Pan");
    lvlParam = apvts.getRawParameterValue (name + "_Level");
    muteParam = apvts.getRawParameterValue (name + "_Mute");
    soloParam = apvts.getRawParameterValue (name + "_Solo");
    
    if (effectChain) effectChain->assignParameters (apvts, name);

    for (auto& send : sends)
    {
        juce::String paramID = name + "_Send_" + send.busName;
        send.gainParam = apvts.getRawParameterValue (paramID);
        juce::String preParamID = name + "_Send_" + send.busName + "_Pre";
        send.prePostParam = apvts.getRawParameterValue (preParamID);
    }

    routeParams.resize(4);
    routeParams[0] = apvts.getRawParameterValue(name + "_Route_Main");
    routeParams[1] = apvts.getRawParameterValue(name + "_Route_Aux1");
    routeParams[2] = apvts.getRawParameterValue(name + "_Route_Aux2");
    routeParams[3] = apvts.getRawParameterValue(name + "_Route_Aux3");
}

void MonoStrip::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", -60.0f, 6.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Pan", 1 }, name + " Pan", -1.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Mute", 1 }, name + " Mute", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Solo", 1 }, name + " Solo", false));

    if (effectChain) effectChain->addParameters (params, name);

    for (auto& send : sends)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Send_" + send.busName, 1 }, name + " Send " + send.busName, -60.0f, 6.0f, -60.0f));
        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Send_" + send.busName + "_Pre", 1 }, name + " Send " + send.busName + " Pre", false));
    }

    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Main", 1 }, name + " Route Main", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux1", 1 }, name + " Route Aux 1", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux2", 1 }, name + " Route Aux 2", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux3", 1 }, name + " Route Aux 3", false));
}

void MonoStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset)
{
    if (panParam) pan = *panParam;
    if (lvlParam) level = juce::Decibels::decibelsToGain (lvlParam->load());

    if (tempBuffer.getNumSamples() != input.getNumSamples())
        tempBuffer.setSize (1, input.getNumSamples(), false, false, true);

    tempBuffer.clear();
    tempBuffer.copyFrom (0, 0, input, inputChannelOffset, 0, input.getNumSamples());

    processSends (tempBuffer, true); // Pre FX+Fader
    processEffects (tempBuffer);
    
    tempBuffer.applyGain (level);
    processSends (tempBuffer, false); // Post FX+Fader

    meter.process (tempBuffer.getReadPointer (0), tempBuffer.getNumSamples());

    // float gainL = 0.5f * (1.0f - pan);
    // float gainR = 0.5f * (1.0f + pan);
    float panRad = (1+pan) * juce::MathConstants<float>::halfPi * 0.5f;
    float gainL = juce::dsp::FastMathApproximations::cos (panRad);
    float gainR = juce::dsp::FastMathApproximations::sin (panRad);

    // Routing
    if (routeParams[0] && *routeParams[0] > 0.5f)
    {
        mixBuffer.addFrom (0, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainL);
        mixBuffer.addFrom (1, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainR);
    }

    if (routeParams[1] && *routeParams[1] > 0.5f && outputBuffer.getNumChannels() >= 4)
    {
        outputBuffer.addFrom (2, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainL);
        outputBuffer.addFrom (3, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainR);
    }
    if (routeParams[2] && *routeParams[2] > 0.5f && outputBuffer.getNumChannels() >= 6)
    {
        outputBuffer.addFrom (4, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainL);
        outputBuffer.addFrom (5, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainR);
    }
    if (routeParams[3] && *routeParams[3] > 0.5f && outputBuffer.getNumChannels() >= 8)
    {
        outputBuffer.addFrom (6, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainL);
        outputBuffer.addFrom (7, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainR);
    }
}

void MonoStrip::clearMeters()
{
    meter.clear();
}

//==============================================================================
StereoReverbStrip::StereoReverbStrip (const juce::String& n) : MixerStrip (n) {}

void StereoReverbStrip::prepare (double sampleRate, int samplesPerBlock)
{
    if (!effectChain) effectChain = std::make_unique<EffectChainDynamics>();
    effectChain->prepare (sampleRate, samplesPerBlock, 2);

    meterL.prepare (sampleRate);
    meterR.prepare (sampleRate);
    tempBuffer.setSize (2, samplesPerBlock);
}

void StereoReverbStrip::assignParameters (juce::AudioProcessorValueTreeState& apvts)
{
    panParam = apvts.getRawParameterValue (name + "_Pan");
    lvlParam = apvts.getRawParameterValue (name + "_Level");
    muteParam = apvts.getRawParameterValue (name + "_Mute");
    soloParam = apvts.getRawParameterValue (name + "_Solo");
    reverb.assignParameters(apvts, name);
    
    if (effectChain) effectChain->assignParameters (apvts, name);

    for (auto& send : sends)
    {
        juce::String paramID = name + "_Send_" + send.busName;
        send.gainParam = apvts.getRawParameterValue (paramID);
        juce::String preParamID = name + "_Send_" + send.busName + "_Pre";
        send.prePostParam = apvts.getRawParameterValue (preParamID);
    }

    routeParams.resize(4);
    routeParams[0] = apvts.getRawParameterValue(name + "_Route_Main");
    routeParams[1] = apvts.getRawParameterValue(name + "_Route_Aux1");
    routeParams[2] = apvts.getRawParameterValue(name + "_Route_Aux2");
    routeParams[3] = apvts.getRawParameterValue(name + "_Route_Aux3");
}

void StereoReverbStrip::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) // NOLINT
{
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", -60.0f, 6.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Pan", 1 }, name + " Pan", -1.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Mute", 1 }, name + " Mute", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Solo", 1 }, name + " Solo", false));
    ConvolReverb::addParameters (params, name, reverb.getImpulseNames().size());

    if (effectChain) effectChain->addParameters (params, name);

    for (auto& send : sends)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Send_" + send.busName, 1 }, name + " Send " + send.busName, -60.0f, 6.0f, -60.0f));
        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Send_" + send.busName + "_Pre", 1 }, name + " Send " + send.busName + " Pre", false));
    }

    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Main", 1 }, name + " Route Main", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux1", 1 }, name + " Route Aux 1", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux2", 1 }, name + " Route Aux 2", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux3", 1 }, name + " Route Aux 3", false));
}

void StereoReverbStrip::setImpulseList (const juce::StringArray& names, const juce::StringArray& resources)
{
    reverb.setImpulseList (names, resources);
}

void StereoReverbStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset)
{
    if (panParam) pan = *panParam;
    if (lvlParam) level = juce::Decibels::decibelsToGain (lvlParam->load());

    if (tempBuffer.getNumSamples() != input.getNumSamples())
        tempBuffer.setSize (2, input.getNumSamples(), false, false, true);

    tempBuffer.clear();
    // Copy mono input to both channels for stereo processing
    tempBuffer.copyFrom (0, 0, input, inputChannelOffset, 0, input.getNumSamples());
    tempBuffer.copyFrom (1, 0, input, inputChannelOffset, 0, input.getNumSamples());

    reverb.process (tempBuffer);
    processSends (tempBuffer, true); // Pre FX+Fader
    processEffects (tempBuffer);
    
    tempBuffer.applyGain (level);
    processSends (tempBuffer, false); // Post FX+Fader

    // Balance using constant-power law
    float panRad = (1.0f + pan) * juce::MathConstants<float>::halfPi * 0.5f;
    float gainL = juce::dsp::FastMathApproximations::cos (panRad);
    float gainR = juce::dsp::FastMathApproximations::sin (panRad);
    
    tempBuffer.applyGain (0, 0, tempBuffer.getNumSamples(), gainL);
    tempBuffer.applyGain (1, 0, tempBuffer.getNumSamples(), gainR);

    meterL.process (tempBuffer.getReadPointer (0), tempBuffer.getNumSamples());
    meterR.process (tempBuffer.getReadPointer (1), tempBuffer.getNumSamples());

    // Routing
    if (routeParams[0] && *routeParams[0] > 0.5f)
        for (int ch = 0; ch < 2; ++ch)
            mixBuffer.addFrom (ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());

    if (routeParams[1] && *routeParams[1] > 0.5f && outputBuffer.getNumChannels() >= 4)
        for (int ch = 0; ch < 2; ++ch)
            outputBuffer.addFrom (2 + ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());

    if (routeParams[2] && *routeParams[2] > 0.5f && outputBuffer.getNumChannels() >= 6)
        for (int ch = 0; ch < 2; ++ch)
            outputBuffer.addFrom (4 + ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());

    if (routeParams[3] && *routeParams[3] > 0.5f && outputBuffer.getNumChannels() >= 8)
        for (int ch = 0; ch < 2; ++ch)
            outputBuffer.addFrom (6 + ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());
}

void StereoReverbStrip::clearMeters()
{
    meterL.clear();
    meterR.clear();
}

//==============================================================================
MonoReverbStrip::MonoReverbStrip (const juce::String& n) : MixerStrip (n) {}

void MonoReverbStrip::prepare (double sampleRate, int samplesPerBlock)
{
    if (!effectChain) effectChain = std::make_unique<EffectChainDynamics>();
    effectChain->prepare (sampleRate, samplesPerBlock, 1);

    reverb.prepare (sampleRate, samplesPerBlock);
    meter.prepare (sampleRate);
    tempBuffer.setSize (1, samplesPerBlock);
}

void MonoReverbStrip::assignParameters (juce::AudioProcessorValueTreeState& apvts)
{
    panParam = apvts.getRawParameterValue (name + "_Pan");
    lvlParam = apvts.getRawParameterValue (name + "_Level");
    muteParam = apvts.getRawParameterValue (name + "_Mute");
    soloParam = apvts.getRawParameterValue (name + "_Solo");
    reverb.assignParameters(apvts, name);
    
    if (effectChain) effectChain->assignParameters (apvts, name);

    for (auto& send : sends)
    {
        juce::String paramID = name + "_Send_" + send.busName;
        send.gainParam = apvts.getRawParameterValue (paramID);
        juce::String preParamID = name + "_Send_" + send.busName + "_Pre";
        send.prePostParam = apvts.getRawParameterValue (preParamID);
    }

    routeParams.resize(4);
    routeParams[0] = apvts.getRawParameterValue(name + "_Route_Main");
    routeParams[1] = apvts.getRawParameterValue(name + "_Route_Aux1");
    routeParams[2] = apvts.getRawParameterValue(name + "_Route_Aux2");
    routeParams[3] = apvts.getRawParameterValue(name + "_Route_Aux3");
}

void MonoReverbStrip::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) // NOLINT
{
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", -60.0f, 6.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Pan", 1 }, name + " Pan", -1.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Mute", 1 }, name + " Mute", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Solo", 1 }, name + " Solo", false));
    ConvolReverb::addParameters (params, name, reverb.getImpulseNames().size());

    if (effectChain) effectChain->addParameters (params, name);

    for (auto& send : sends)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Send_" + send.busName, 1 }, name + " Send " + send.busName, -60.0f, 6.0f, -60.0f));
        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Send_" + send.busName + "_Pre", 1 }, name + " Send " + send.busName + " Pre", false));
    }

    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Main", 1 }, name + " Route Main", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux1", 1 }, name + " Route Aux 1", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux2", 1 }, name + " Route Aux 2", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux3", 1 }, name + " Route Aux 3", false));
}

void MonoReverbStrip::setImpulseList (const juce::StringArray& names, const juce::StringArray& resources)
{
    reverb.setImpulseList (names, resources);
}

void MonoReverbStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset)
{
    if (panParam) pan = *panParam;
    if (lvlParam) level = juce::Decibels::decibelsToGain (lvlParam->load());

    if (tempBuffer.getNumSamples() != input.getNumSamples())
        tempBuffer.setSize (1, input.getNumSamples(), false, false, true);

    tempBuffer.clear();
    tempBuffer.copyFrom (0, 0, input, inputChannelOffset, 0, input.getNumSamples());

    reverb.process (tempBuffer);
    processSends (tempBuffer, true); // Pre FX+Fader
    processEffects (tempBuffer);
    
    tempBuffer.applyGain (level);
    processSends (tempBuffer, false); // Post FX+Fader

    meter.process (tempBuffer.getReadPointer (0), tempBuffer.getNumSamples());

    // Balance using constant-power law
    float panRad = (1.0f + pan) * juce::MathConstants<float>::halfPi * 0.5f;
    float gainL = juce::dsp::FastMathApproximations::cos (panRad);
    float gainR = juce::dsp::FastMathApproximations::sin (panRad);
    
    // Routing
    if (routeParams[0] && *routeParams[0] > 0.5f)
    {
        mixBuffer.addFrom (0, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainL);
        mixBuffer.addFrom (1, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainR);
    }

    if (routeParams[1] && *routeParams[1] > 0.5f && outputBuffer.getNumChannels() >= 4)
    {
        outputBuffer.addFrom (2, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainL);
        outputBuffer.addFrom (3, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainR);
    }
    if (routeParams[2] && *routeParams[2] > 0.5f && outputBuffer.getNumChannels() >= 6)
    {
        outputBuffer.addFrom (4, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainL);
        outputBuffer.addFrom (5, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainR);
    }
    if (routeParams[3] && *routeParams[3] > 0.5f && outputBuffer.getNumChannels() >= 8)
    {
        outputBuffer.addFrom (6, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainL);
        outputBuffer.addFrom (7, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainR);
    }
}

void MonoReverbStrip::clearMeters()
{
    meter.clear();
}

//==============================================================================
BusStrip::BusStrip (const juce::String& n) : MixerStrip (n) {}

void BusStrip::prepare (double sampleRate, int samplesPerBlock)
{
    if (effectChain)
        effectChain->prepare (sampleRate, samplesPerBlock, 2);

    meterL.prepare (sampleRate);
    meterR.prepare (sampleRate);
    tempBuffer.setSize (2, samplesPerBlock);
    busBuffer.setSize (2, samplesPerBlock);
}

void BusStrip::assignParameters (juce::AudioProcessorValueTreeState& apvts)
{
    panParam = apvts.getRawParameterValue (name + "_Pan");
    wParam = apvts.getRawParameterValue (name + "_Width");
    lvlParam = apvts.getRawParameterValue (name + "_Level");
    muteParam = apvts.getRawParameterValue (name + "_Mute");
    soloParam = apvts.getRawParameterValue (name + "_Solo");
    
    if (effectChain) effectChain->assignParameters (apvts, name);

    for (auto& send : sends)
    {
        juce::String paramID = name + "_Send_" + send.busName;
        send.gainParam = apvts.getRawParameterValue (paramID);
        juce::String preParamID = name + "_Send_" + send.busName + "_Pre";
        send.prePostParam = apvts.getRawParameterValue (preParamID);
    }

    routeParams.resize(4);
    routeParams[0] = apvts.getRawParameterValue(name + "_Route_Main");
    routeParams[1] = apvts.getRawParameterValue(name + "_Route_Aux1");
    routeParams[2] = apvts.getRawParameterValue(name + "_Route_Aux2");
    routeParams[3] = apvts.getRawParameterValue(name + "_Route_Aux3");
}

void BusStrip::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Pan", 1 }, name + " Pan", -1.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Width", 1 }, name + " Width", 0.0f, 2.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", -60.0f, 6.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Mute", 1 }, name + " Mute", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Solo", 1 }, name + " Solo", false));

    if (effectChain) effectChain->addParameters (params, name);

    for (auto& send : sends)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Send_" + send.busName, 1 }, name + " Send " + send.busName, -60.0f, 6.0f, -60.0f));
        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Send_" + send.busName + "_Pre", 1 }, name + " Send " + send.busName + " Pre", false));
    }

    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Main", 1 }, name + " Route Main", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux1", 1 }, name + " Route Aux 1", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux2", 1 }, name + " Route Aux 2", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Route_Aux3", 1 }, name + " Route Aux 3", false));
}

void BusStrip::addInput (const juce::AudioBuffer<float>& source, float gain)
{
    // Ensure busBuffer is large enough to hold the source
    if (busBuffer.getNumSamples() < source.getNumSamples())
        busBuffer.setSize (2, source.getNumSamples(), true, true, true);

    // Mix source into busBuffer. Source can be mono or stereo.
    // busBuffer is stereo.
    if (source.getNumChannels() == 1)
    {
        busBuffer.addFrom (0, 0, source, 0, 0, source.getNumSamples(), gain);
        busBuffer.addFrom (1, 0, source, 0, 0, source.getNumSamples(), gain);
    }
    else
    {
        for (int i = 0; i < 2 && i < source.getNumChannels(); ++i)
            busBuffer.addFrom (i, 0, source, i, 0, source.getNumSamples(), gain);
    }
}

void BusStrip::clearBusBuffer()
{
    busBuffer.clear();
}

void BusStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset)
{
    // Bus ignores 'input' (sampler output) and uses 'busBuffer' (sends)
    juce::ignoreUnused (input, inputChannelOffset);

    if (panParam) pan = *panParam;
    if (wParam) width = *wParam;
    if (lvlParam) level = juce::Decibels::decibelsToGain (lvlParam->load());

    int numSamples = outputBuffer.getNumSamples();

    if (tempBuffer.getNumSamples() != numSamples)
        tempBuffer.setSize (2, numSamples, false, false, true);

    // Ensure busBuffer has enough data (it should from addInput, but safety first)
    if (busBuffer.getNumSamples() < numSamples)
        busBuffer.setSize(2, numSamples, true, true, true);

    for (int i = 0; i < 2; ++i)
        tempBuffer.copyFrom (i, 0, busBuffer, i, 0, numSamples);

    // Width processing (Standard M/S width control for stereo sources)
    if (std::abs(width - 1.0f) > 0.001f)
    {
        auto* l = tempBuffer.getWritePointer(0);
        auto* r = tempBuffer.getWritePointer(1);
        for (int i = 0; i < numSamples; ++i)
        {
            float mid = (l[i] + r[i]) * 0.5f;
            float side = (l[i] - r[i]) * 0.5f * width;
            l[i] = mid + side;
            r[i] = mid - side;
        }
    }

    processSends (tempBuffer, true); // Pre FX+Fader
    processEffects (tempBuffer);
    
    tempBuffer.applyGain (level);
    processSends (tempBuffer, false); // Post FX+Fader

    // Balance using Linear law (Classical Stereo)
    float gainL = 0.5f * (1.0f - pan);
    float gainR = 0.5f * (1.0f + pan);
    
    tempBuffer.applyGain (0, 0, numSamples, gainL);
    tempBuffer.applyGain (1, 0, numSamples, gainR);

    meterL.process (tempBuffer.getReadPointer (0), numSamples);
    meterR.process (tempBuffer.getReadPointer (1), numSamples);

    // Routing
    if (routeParams[0] && *routeParams[0] > 0.5f)
        for (int ch = 0; ch < 2; ++ch)
            mixBuffer.addFrom (ch, 0, tempBuffer, ch, 0, numSamples);

    if (routeParams[1] && *routeParams[1] > 0.5f && outputBuffer.getNumChannels() >= 4)
        for (int ch = 0; ch < 2; ++ch)
            outputBuffer.addFrom (2 + ch, 0, tempBuffer, ch, 0, numSamples);

    if (routeParams[2] && *routeParams[2] > 0.5f && outputBuffer.getNumChannels() >= 6)
        for (int ch = 0; ch < 2; ++ch)
            outputBuffer.addFrom (4 + ch, 0, tempBuffer, ch, 0, numSamples);

    if (routeParams[3] && *routeParams[3] > 0.5f && outputBuffer.getNumChannels() >= 8)
        for (int ch = 0; ch < 2; ++ch)
            outputBuffer.addFrom (6 + ch, 0, tempBuffer, ch, 0, numSamples);
}

void BusStrip::clearMeters()
{
    meterL.clear();
    meterR.clear();
}

//==============================================================================
MasterStrip::MasterStrip (const juce::String& n) : MixerStrip (n) {}

void MasterStrip::prepare (double sampleRate, int samplesPerBlock)
{
    if (!effectChain) effectChain = std::make_unique<EffectChainDynamics>();
    effectChain->prepare (sampleRate, samplesPerBlock, 2);

    meterL.prepare (sampleRate);
    meterR.prepare (sampleRate);
    tempBuffer.setSize (2, samplesPerBlock);
}

void MasterStrip::assignParameters (juce::AudioProcessorValueTreeState& apvts)
{
    panParam = apvts.getRawParameterValue (name + "_Pan");
    wParam = apvts.getRawParameterValue (name + "_Width");
    lvlParam = apvts.getRawParameterValue (name + "_Level");
    muteParam = apvts.getRawParameterValue (name + "_Mute");
    // Master doesn't really need solo, but we keep the pointer null or unused if not needed.
    // We'll bind it just in case to avoid null checks failing if we added it to params.
    soloParam = apvts.getRawParameterValue (name + "_Solo"); 
    
    if (effectChain) effectChain->assignParameters (apvts, name);
}

void MasterStrip::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Pan", 1 }, name + " Pan", -1.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Width", 1 }, name + " Width", 0.0f, 2.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", -60.0f, 6.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Mute", 1 }, name + " Mute", false));

    if (effectChain) effectChain->addParameters (params, name);
}

void MasterStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset)
{
    // MasterStrip process takes the mixBuffer (input) and writes to outputBuffer (channels 0-1)
    // It behaves like StereoStrip: copies input to temp, processes, adds to output.
    // mixBuffer arg is unused here as Master is the final stage for Main Output.

    // We can reuse StereoStrip logic if we cast or just copy the code. 
    // Since I cannot easily call StereoStrip::process on *this without inheritance tricks or code duplication,
    // I will duplicate the logic here for clarity and safety.
    
    if (panParam) pan = *panParam;
    if (wParam) width = *wParam;
    if (lvlParam) level = juce::Decibels::decibelsToGain (lvlParam->load());

    if (tempBuffer.getNumSamples() != input.getNumSamples())
        tempBuffer.setSize (2, input.getNumSamples(), false, false, true);

    tempBuffer.clear();
    for (int i = 0; i < 2; ++i)
        tempBuffer.copyFrom (i, 0, input, inputChannelOffset + i, 0, input.getNumSamples());

    // Width processing
    if (std::abs(width - 1.0f) > 0.001f)
    {
        auto* l = tempBuffer.getWritePointer(0);
        auto* r = tempBuffer.getWritePointer(1);
        for (int i = 0; i < tempBuffer.getNumSamples(); ++i)
        {
            float mid = (l[i] + r[i]) * 0.5f;
            float side = (l[i] - r[i]) * 0.5f * width;
            l[i] = mid + side;
            r[i] = mid - side;
        }
    }

    processEffects (tempBuffer);
    tempBuffer.applyGain (level);

    // Balance
    float gainL = 0.5f * (1.0f - pan);
    float gainR = 0.5f * (1.0f + pan);
    
    tempBuffer.applyGain (0, 0, tempBuffer.getNumSamples(), gainL);
    tempBuffer.applyGain (1, 0, tempBuffer.getNumSamples(), gainR);

    meterL.process (tempBuffer.getReadPointer (0), tempBuffer.getNumSamples());
    meterR.process (tempBuffer.getReadPointer (1), tempBuffer.getNumSamples());

    for (int ch = 0; ch < 2; ++ch)
        outputBuffer.addFrom (ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());
}

void MasterStrip::clearMeters()
{
    meterL.clear();
    meterR.clear();
}