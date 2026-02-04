/*
  ==============================================================================

    Mixer.cpp

  ==============================================================================
*/

#include "Mixer.h"

//==============================================================================
void MixerStrip::processEffects (juce::AudioBuffer<float>& buffer)
{
    eq.checkParameters();
    comp.checkParameters();
    tube.checkParameters();

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

//==============================================================================
AmbisonicStrip::AmbisonicStrip (const juce::String& n) : MixerStrip (n) {}

void AmbisonicStrip::prepare (double sampleRate, int samplesPerBlock)
{
    eq.prepare (sampleRate, 2);
    comp.prepare (sampleRate, 2);
    tube.prepare (sampleRate);
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
    orderParam = apvts.getRawParameterValue (name + "_Order");
    eq.assignParameters (apvts, name);
    comp.assignParameters (apvts, name);
    tube.assignParameters (apvts, name);
}

void AmbisonicStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset)
{
    if (azParam) ambix.setAzimuth (*azParam);
    if (elParam) ambix.setElevation (*elParam);
    if (wParam) ambix.setWidth (*wParam);
    if (lvlParam) ambix.setLevel (*lvlParam);

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

    meterL.process (tempBuffer.getReadPointer (0), tempBuffer.getNumSamples());
    meterR.process (tempBuffer.getReadPointer (1), tempBuffer.getNumSamples());

    for (int ch = 0; ch < 2; ++ch)
        output.addFrom (ch, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());
}

//==============================================================================
MSStrip::MSStrip (const juce::String& n) : MixerStrip (n) {}

void MSStrip::prepare (double sampleRate, int samplesPerBlock)
{
    eq.prepare (sampleRate, 2);
    comp.prepare (sampleRate, 2);
    tube.prepare (sampleRate);
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
    orderParam = apvts.getRawParameterValue (name + "_Order");
    eq.assignParameters (apvts, name);
    comp.assignParameters (apvts, name);
    tube.assignParameters (apvts, name);
}

void MSStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset)
{
    if (panParam) pan = *panParam;
    if (wParam) width = *wParam;
    if (lvlParam) level = *lvlParam;

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

//==============================================================================
StereoStrip::StereoStrip (const juce::String& n) : MixerStrip (n) {}

void StereoStrip::prepare (double sampleRate, int samplesPerBlock)
{
    eq.prepare (sampleRate, 2);
    comp.prepare (sampleRate, 2);
    tube.prepare (sampleRate);
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
    orderParam = apvts.getRawParameterValue (name + "_Order");
    eq.assignParameters (apvts, name);
    comp.assignParameters (apvts, name);
    tube.assignParameters (apvts, name);
}

void StereoStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset)
{
    if (panParam) pan = *panParam;
    if (wParam) width = *wParam;
    if (lvlParam) level = *lvlParam;

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

//==============================================================================
MonoStrip::MonoStrip (const juce::String& n) : MixerStrip (n) {}

void MonoStrip::prepare (double sampleRate, int samplesPerBlock)
{
    eq.prepare (sampleRate, 1);
    comp.prepare (sampleRate, 1);
    tube.prepare (sampleRate);
    meter.prepare (sampleRate);
    tempBuffer.setSize (1, samplesPerBlock);
}

void MonoStrip::assignParameters (juce::AudioProcessorValueTreeState& apvts)
{
    panParam = apvts.getRawParameterValue (name + "_Pan");
    lvlParam = apvts.getRawParameterValue (name + "_Level");
    muteParam = apvts.getRawParameterValue (name + "_Mute");
    soloParam = apvts.getRawParameterValue (name + "_Solo");
    orderParam = apvts.getRawParameterValue (name + "_Order");
    eq.assignParameters (apvts, name);
    comp.assignParameters (apvts, name);
    tube.assignParameters (apvts, name);
}

void MonoStrip::process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset)
{
    if (panParam) pan = *panParam;
    if (lvlParam) level = *lvlParam;

    tempBuffer.clear();
    tempBuffer.copyFrom (0, 0, input, inputChannelOffset, 0, input.getNumSamples());

    processEffects (tempBuffer);
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

//==============================================================================
void Mixer::prepare (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentSamplesPerBlock = samplesPerBlock;
    for (auto& strip : strips)
        strip->prepare (sampleRate, samplesPerBlock);
}

void Mixer::loadFromXml (const void* xmlData, int xmlSize)
{
    if (xmlData == nullptr || xmlSize <= 0)
        return;

    juce::XmlDocument doc (juce::String::createStringFromData (xmlData, xmlSize));
    auto root = doc.getDocumentElement();

    if (root == nullptr || ! root->hasTagName ("Mappings"))
        return;

    auto* mixerNode = root->getChildByName ("Mixer");
    if (mixerNode == nullptr)
        return;

    strips.clear();

    for (auto* child : mixerNode->getChildIterator())
    {
        if (child->hasTagName ("Group"))
        {
            juce::String type = child->getStringAttribute ("type");
            juce::String name = child->getStringAttribute ("name");
            juce::String imgName = child->getStringAttribute ("img");

            std::unique_ptr<MixerStrip> newStrip;

            if (type.equalsIgnoreCase ("ambisonic"))
                newStrip = std::make_unique<AmbisonicStrip> (name);
            else if (type.equalsIgnoreCase ("stereo"))
                newStrip = std::make_unique<StereoStrip> (name);
            else if (type.equalsIgnoreCase ("ms"))
                newStrip = std::make_unique<MSStrip> (name);
            else if (type.equalsIgnoreCase ("mono"))
                newStrip = std::make_unique<MonoStrip> (name);

            if (newStrip != nullptr)
            {
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
                strips.push_back (std::move (newStrip));
            }
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
}

void Mixer::processBlock (const juce::AudioBuffer<float>& inputBuffer, juce::AudioBuffer<float>& outputBuffer)
{
    int currentInputChannel = 0;
    int totalInputChannels = inputBuffer.getNumChannels();

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
                strip->process (inputBuffer, outputBuffer, currentInputChannel);
            currentInputChannel += needed;
        }
    }
}