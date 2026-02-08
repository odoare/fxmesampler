/*
  ==============================================================================

    Mixer.cpp

  ==============================================================================
*/

#include "Mixer.h"

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

void MixerStrip::processSends (juce::AudioBuffer<float>& buffer)
{
    for (auto& send : sends)
    {
        if (send.gainParam) send.currentGain = juce::Decibels::decibelsToGain (send.gainParam->load());
        if (send.bus && send.currentGain > 0.0001f)
            send.bus->addInput (buffer, send.currentGain);
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
    }
}

void AmbisonicStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset)
{
    if (azParam) ambix.setAzimuth (*azParam);
    if (elParam) ambix.setElevation (*elParam);
    if (wParam) ambix.setWidth (*wParam);
    if (lvlParam) ambix.setLevel (juce::Decibels::decibelsToGain (lvlParam->load()));

    if (tempBuffer.getNumSamples() != input.getNumSamples())
        tempBuffer.setSize (2, input.getNumSamples(), false, false, true);

    tempBuffer.clear();
    // AmbixToMS expects 4 channels starting at 0 in its input buffer logic, 
    // but we pass a subset or use pointers. AmbixToMS::process takes buffers.
    // We need to create a proxy buffer for the input channels.
    juce::AudioBuffer<float> proxyBuffer;
    // const_cast is ugly but AudioBuffer doesn't have a const-correct subset creation easily without copying
    // However, AmbixToMS::process takes const input.
    // We can create a buffer that points to the data.
    std::vector<const float*> readPointers;
    for (int i = 0; i < 4; ++i)
        readPointers.push_back (input.getReadPointer (inputChannelOffset + i));
    
    juce::AudioBuffer<float> inputSubset (const_cast<float**>(readPointers.data()), 4, input.getNumSamples());

    ambix.process (inputSubset, tempBuffer); // Adds to tempBuffer
    processEffects (tempBuffer);
    processSends (tempBuffer);

    meterL.process (tempBuffer.getReadPointer (0), tempBuffer.getNumSamples());
    meterR.process (tempBuffer.getReadPointer (1), tempBuffer.getNumSamples());

    for (int ch = 0; ch < 2; ++ch)
        output.addFrom (ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());
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
    }
}

void MSStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset)
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

    processEffects (tempBuffer);
    processSends (tempBuffer);
    tempBuffer.applyGain (level);

    // Balance using constant-power law
    float panRad = (1.0f + pan) * juce::MathConstants<float>::halfPi * 0.5f;
    float gainL = juce::dsp::FastMathApproximations::cos (panRad);
    float gainR = juce::dsp::FastMathApproximations::sin (panRad);
    tempBuffer.applyGain (0, 0, tempBuffer.getNumSamples(), gainL);
    tempBuffer.applyGain (1, 0, tempBuffer.getNumSamples(), gainR);

    meterL.process (tempBuffer.getReadPointer (0), tempBuffer.getNumSamples());
    meterR.process (tempBuffer.getReadPointer (1), tempBuffer.getNumSamples());

    for (int ch = 0; ch < 2; ++ch)
        output.addFrom (ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());
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
    }
}

void StereoStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset)
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

    processEffects (tempBuffer);
    processSends (tempBuffer);
    tempBuffer.applyGain (level);

    // Balance using Linear law (Classical Stereo)
    float gainL = 0.5f * (1.0f - pan);
    float gainR = 0.5f * (1.0f + pan);
    
    tempBuffer.applyGain (0, 0, tempBuffer.getNumSamples(), gainL);
    tempBuffer.applyGain (1, 0, tempBuffer.getNumSamples(), gainR);

    meterL.process (tempBuffer.getReadPointer (0), tempBuffer.getNumSamples());
    meterR.process (tempBuffer.getReadPointer (1), tempBuffer.getNumSamples());

    for (int ch = 0; ch < 2; ++ch)
        output.addFrom (ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());
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
    }
}

void MonoStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset)
{
    if (panParam) pan = *panParam;
    if (lvlParam) level = juce::Decibels::decibelsToGain (lvlParam->load());

    if (tempBuffer.getNumSamples() != input.getNumSamples())
        tempBuffer.setSize (1, input.getNumSamples(), false, false, true);

    tempBuffer.clear();
    tempBuffer.copyFrom (0, 0, input, inputChannelOffset, 0, input.getNumSamples());

    processEffects (tempBuffer);
    processSends (tempBuffer);
    tempBuffer.applyGain (level);

    meter.process (tempBuffer.getReadPointer (0), tempBuffer.getNumSamples());

    // float gainL = 0.5f * (1.0f - pan);
    // float gainR = 0.5f * (1.0f + pan);
    float panRad = (1+pan) * juce::MathConstants<float>::halfPi * 0.5f;
    float gainL = juce::dsp::FastMathApproximations::cos (panRad);
    float gainR = juce::dsp::FastMathApproximations::sin (panRad);

    output.addFrom (0, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainL);
    output.addFrom (1, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainR);
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
    
    if (effectChain) effectChain->assignParameters (apvts, name);

    for (auto& send : sends)
    {
        juce::String paramID = name + "_Send_" + send.busName;
        send.gainParam = apvts.getRawParameterValue (paramID);
    }
}

void StereoReverbStrip::loadImpulse (const void* data, size_t size)
{
    reverb.loadImpulse (data, size);
}

void StereoReverbStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset)
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
    processEffects (tempBuffer);
    processSends (tempBuffer);
    tempBuffer.applyGain (level);

    // Balance using constant-power law
    float panRad = (1.0f + pan) * juce::MathConstants<float>::halfPi * 0.5f;
    float gainL = juce::dsp::FastMathApproximations::cos (panRad);
    float gainR = juce::dsp::FastMathApproximations::sin (panRad);
    
    tempBuffer.applyGain (0, 0, tempBuffer.getNumSamples(), gainL);
    tempBuffer.applyGain (1, 0, tempBuffer.getNumSamples(), gainR);

    meterL.process (tempBuffer.getReadPointer (0), tempBuffer.getNumSamples());
    meterR.process (tempBuffer.getReadPointer (1), tempBuffer.getNumSamples());

    for (int ch = 0; ch < 2; ++ch)
        output.addFrom (ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());
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
    
    if (effectChain) effectChain->assignParameters (apvts, name);

    for (auto& send : sends)
    {
        juce::String paramID = name + "_Send_" + send.busName;
        send.gainParam = apvts.getRawParameterValue (paramID);
    }
}

void MonoReverbStrip::loadImpulse (const void* data, size_t size)
{
    reverb.loadImpulse (data, size);
}

void MonoReverbStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset)
{
    if (panParam) pan = *panParam;
    if (lvlParam) level = juce::Decibels::decibelsToGain (lvlParam->load());

    if (tempBuffer.getNumSamples() != input.getNumSamples())
        tempBuffer.setSize (1, input.getNumSamples(), false, false, true);

    tempBuffer.clear();
    tempBuffer.copyFrom (0, 0, input, inputChannelOffset, 0, input.getNumSamples());

    reverb.process (tempBuffer);
    processEffects (tempBuffer);
    processSends (tempBuffer);
    tempBuffer.applyGain (level);

    meter.process (tempBuffer.getReadPointer (0), tempBuffer.getNumSamples());

    // Balance using constant-power law
    float panRad = (1.0f + pan) * juce::MathConstants<float>::halfPi * 0.5f;
    float gainL = juce::dsp::FastMathApproximations::cos (panRad);
    float gainR = juce::dsp::FastMathApproximations::sin (panRad);
    
    output.addFrom (0, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainL);
    output.addFrom (1, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples(), gainR);
}

void MonoReverbStrip::clearMeters()
{
    meter.clear();
}

//==============================================================================
BusStrip::BusStrip (const juce::String& n) : MixerStrip (n) {}

void BusStrip::prepare (double sampleRate, int samplesPerBlock)
{
    if (!effectChain) effectChain = std::make_unique<EffectChainDynamics>();
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
    }
}

void BusStrip::addInput (const juce::AudioBuffer<float>& source, float gain)
{
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

void BusStrip::loadImpulse (const void* data, size_t size)
{
    reverb.loadImpulse (data, size);
}

void BusStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset)
{
    // Bus ignores 'input' (sampler output) and uses 'busBuffer' (sends)
    juce::ignoreUnused (input, inputChannelOffset);

    if (panParam) pan = *panParam;
    if (wParam) width = *wParam;
    if (lvlParam) level = juce::Decibels::decibelsToGain (lvlParam->load());

    if (tempBuffer.getNumSamples() != busBuffer.getNumSamples())
        tempBuffer.setSize (2, busBuffer.getNumSamples(), false, false, true);

    tempBuffer.makeCopyOf (busBuffer);

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

    if (isReverb)
        reverb.process (tempBuffer);

    processEffects (tempBuffer);
    processSends (tempBuffer); // Bus to Bus sends possible if ordered correctly
    tempBuffer.applyGain (level);

    // Balance using Linear law (Classical Stereo)
    float gainL = 0.5f * (1.0f - pan);
    float gainR = 0.5f * (1.0f + pan);
    
    tempBuffer.applyGain (0, 0, tempBuffer.getNumSamples(), gainL);
    tempBuffer.applyGain (1, 0, tempBuffer.getNumSamples(), gainR);

    meterL.process (tempBuffer.getReadPointer (0), tempBuffer.getNumSamples());
    meterR.process (tempBuffer.getReadPointer (1), tempBuffer.getNumSamples());

    for (int ch = 0; ch < 2; ++ch)
        output.addFrom (ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());
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

void MasterStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset)
{
    // MasterStrip process takes the mixBuffer (input) and writes to outputBuffer (output)
    // It behaves like StereoStrip: copies input to temp, processes, adds to output.
    // Since output is cleared before Mixer::processBlock finishes, this works as "set".

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
        output.addFrom (ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());
}

void MasterStrip::clearMeters()
{
    meterL.clear();
    meterR.clear();
}

//==============================================================================
void Mixer::prepare (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentSamplesPerBlock = samplesPerBlock;
    mixBuffer.setSize (2, samplesPerBlock); // Stereo mix bus
    for (auto& strip : strips)
        strip->prepare (sampleRate, samplesPerBlock);
    masterStrip.prepare (sampleRate, samplesPerBlock);
}

void Mixer::loadFromXml (const void* xmlData, int xmlSize)
{
    if (xmlData == nullptr || xmlSize <= 0)
        return;

    juce::XmlDocument doc (juce::String::createStringFromData (xmlData, xmlSize));
    auto root = doc.getDocumentElement();

    if (root == nullptr || ! root->hasTagName ("Mappings"))
        return;

    auto* masterNode = root->getChildByName ("Master");
    if (masterNode != nullptr)
    {
        juce::String imgName = masterNode->getStringAttribute ("img");
        juce::String colorStr = masterNode->getStringAttribute ("color");

        if (colorStr.isNotEmpty())
        {
            juce::StringArray tokens;
            tokens.addTokens (colorStr, ",", "");
            if (tokens.size() == 3)
            {
                masterStrip.setColor (juce::Colour::fromRGB (
                    (juce::uint8) tokens[0].getIntValue(),
                    (juce::uint8) tokens[1].getIntValue(),
                    (juce::uint8) tokens[2].getIntValue()));
            }
            else
            {
                masterStrip.setColor (juce::Colour::fromString (colorStr));
            }
        }

        if (imgName.isNotEmpty())
        {
            juce::String resourceName = imgName.replaceCharacter ('.', '_').replaceCharacter (' ', '_');
            int dataSize = 0;
            const char* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), dataSize);

            if (data == nullptr)
            {
                for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
                    if (resourceName.equalsIgnoreCase (BinaryData::namedResourceList[i]))
                        { data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], dataSize); break; }
            }

            if (data != nullptr)
                masterStrip.setImage (juce::ImageCache::getFromMemory (data, dataSize));
        }
    }

    auto* mixerNode = root->getChildByName ("Mixer");
    if (mixerNode == nullptr)
        return;

    strips.clear();

    for (auto* child : mixerNode->getChildIterator())
    {
        std::unique_ptr<MixerStrip> newStrip;
        juce::String type = child->getStringAttribute ("type");
        juce::String name = child->getStringAttribute ("name");
        juce::String chainType = child->getStringAttribute ("effectChain", "dynamics");

        if (child->hasTagName ("Strip"))
        {
            if (type.equalsIgnoreCase ("ambisonic"))
                newStrip = std::make_unique<AmbisonicStrip> (name);
            else if (type.equalsIgnoreCase ("stereo"))
                newStrip = std::make_unique<StereoStrip> (name);
            else if (type.equalsIgnoreCase ("ms"))
                newStrip = std::make_unique<MSStrip> (name);
            else if (type.equalsIgnoreCase ("mono"))
                newStrip = std::make_unique<MonoStrip> (name);
            else if (type.equalsIgnoreCase ("stereoreverb"))
                newStrip = std::make_unique<StereoReverbStrip> (name);
            else if (type.equalsIgnoreCase ("reverb"))
                newStrip = std::make_unique<MonoReverbStrip> (name);
        }
        else if (child->hasTagName ("Bus"))
        {
            if (type.equalsIgnoreCase ("dynamics"))
                newStrip = std::make_unique<BusStrip> (name);
            else if (type.equalsIgnoreCase ("stereoReverb"))
            {
                auto bus = std::make_unique<BusStrip> (name);
                bus->setReverbMode (true);
                newStrip = std::move (bus);
            }
        }

        if (newStrip != nullptr)
        {
            if (chainType.equalsIgnoreCase ("dynamics"))
            {
                // Default, already handled by constructor/prepare logic if we want, 
                // but we can explicitly set it here if we add a setter.
            }
            juce::String imgName = child->getStringAttribute ("img");
            juce::String irName = child->getStringAttribute ("resource");
            juce::String colorStr = child->getStringAttribute ("color");

                if (colorStr.isNotEmpty())
                {
                    juce::StringArray tokens;
                    tokens.addTokens (colorStr, ",", "");
                    if (tokens.size() == 3)
                    {
                        newStrip->setColor (juce::Colour::fromRGB (
                            (juce::uint8) tokens[0].getIntValue(),
                            (juce::uint8) tokens[1].getIntValue(),
                            (juce::uint8) tokens[2].getIntValue()));
                    }
                    else
                    {
                        newStrip->setColor (juce::Colour::fromString (colorStr));
                    }
                }

                if (imgName.isNotEmpty())
                {
                    juce::String resourceName = imgName.replaceCharacter ('.', '_').replaceCharacter (' ', '_');
                    int dataSize = 0;
                    const char* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), dataSize);

                    if (data == nullptr)
                    {
                        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
                            if (resourceName.equalsIgnoreCase (BinaryData::namedResourceList[i]))
                                { data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], dataSize); break; }
                    }

                    if (data != nullptr)
                        newStrip->setImage (juce::ImageCache::getFromMemory (data, dataSize));
                }

                if (irName.isNotEmpty())
                {
                    if (auto* rs = dynamic_cast<StereoReverbStrip*>(newStrip.get()))
                    {
                        juce::String resourceName = irName.replaceCharacter ('.', '_').replaceCharacter (' ', '_');
                        int dataSize = 0;
                        const char* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), dataSize);

                        if (data == nullptr)
                        {
                            for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
                                if (resourceName.equalsIgnoreCase (BinaryData::namedResourceList[i]))
                                    { data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], dataSize); break; }
                        }
                        if (data != nullptr) rs->loadImpulse (data, dataSize);
                    }
                    else if (auto* rs = dynamic_cast<MonoReverbStrip*>(newStrip.get()))
                    {
                        juce::String resourceName = irName.replaceCharacter ('.', '_').replaceCharacter (' ', '_');
                        int dataSize = 0;
                        const char* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), dataSize);

                        if (data == nullptr)
                        {
                            for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
                                if (resourceName.equalsIgnoreCase (BinaryData::namedResourceList[i]))
                                    { data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], dataSize); break; }
                        }
                        if (data != nullptr) rs->loadImpulse (data, dataSize);
                    }
                    else if (auto* bs = dynamic_cast<BusStrip*>(newStrip.get()))
                    {
                        juce::String resourceName = irName.replaceCharacter ('.', '_').replaceCharacter (' ', '_');
                        int dataSize = 0;
                        const char* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), dataSize);

                        if (data == nullptr)
                        {
                            for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
                                if (resourceName.equalsIgnoreCase (BinaryData::namedResourceList[i]))
                                    { data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], dataSize); break; }
                        }
                        if (data != nullptr) bs->loadImpulse (data, dataSize);
                    }
                }
                strips.push_back (std::move (newStrip));
        }
    }

    // Link sends
    std::vector<BusStrip*> buses;
    for (auto& strip : strips)
        if (auto* bus = dynamic_cast<BusStrip*>(strip.get()))
            buses.push_back (bus);

    for (auto& strip : strips)
    {
        for (auto* bus : buses)
        {
            if (strip.get() != bus) // Don't send to self
                strip->addSend (bus->getName(), bus);
        }
    }

    // Re-prepare if we are already running
    if (currentSampleRate > 0)
        prepare (currentSampleRate, currentSamplesPerBlock);
}

void Mixer::assignParameters (juce::AudioProcessorValueTreeState& apvts)
{
    for (auto& strip : strips)
        strip->assignParameters (apvts);
    masterStrip.assignParameters (apvts);
}

void Mixer::processBlock (const juce::AudioBuffer<float>& inputBuffer, juce::AudioBuffer<float>& outputBuffer)
{
    int currentInputChannel = 0;
    int totalInputChannels = inputBuffer.getNumChannels();

    // Ensure mixBuffer is ready
    if (mixBuffer.getNumChannels() != 2 || mixBuffer.getNumSamples() != outputBuffer.getNumSamples())
        mixBuffer.setSize (2, outputBuffer.getNumSamples());
    mixBuffer.clear();

    for (auto& strip : strips)
        if (auto* bus = dynamic_cast<BusStrip*>(strip.get()))
            bus->clearBusBuffer();

    bool anySolo = false;
    for (auto& strip : strips)
        if (strip->isSolo()) { anySolo = true; break; }

    for (auto& strip : strips)
    {
        int needed = strip->getNumInputChannels();
        if (currentInputChannel + needed <= totalInputChannels)
        {
            bool shouldProcess = true;
            if (anySolo) shouldProcess = strip->isSolo();
            else         shouldProcess = ! strip->isMute();

            if (shouldProcess)
                strip->process (inputBuffer, mixBuffer, currentInputChannel); // Sum to mixBuffer
            else
                strip->clearMeters();
            currentInputChannel += needed;
        }
    }

    // Process Master Chain
    // Master takes mixBuffer (stereo) and adds to outputBuffer (which is cleared by processor)
    if (! masterStrip.isMute())
        masterStrip.process (mixBuffer, outputBuffer, 0);
    else
        masterStrip.clearMeters();
}