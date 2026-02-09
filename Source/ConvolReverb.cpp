/*
  ==============================================================================

    ConvolReverb.cpp

  ==============================================================================
*/

#include "ConvolReverb.h"

ConvolReverb::ConvolReverb()
{
}

ConvolReverb::~ConvolReverb()
{
}

void ConvolReverb::prepare (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.Reset();
    juce::ignoreUnused (samplesPerBlock);
}

void ConvolReverb::process (juce::AudioBuffer<float>& buffer)
{
    checkParameters(); // Update parameters before processing
    
    juce::ScopedLock sl (lock);

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    if ((int) wdlInputData.size() < numChannels) wdlInputData.resize (numChannels);
    if ((int) wdlInputPtrs.size() < numChannels) wdlInputPtrs.resize (numChannels);

    for (int c = 0; c < numChannels; ++c)
    {
        wdlInputData[c].resize (numSamples);
        auto* src = buffer.getReadPointer (c);
        auto* dst = wdlInputData[c].data();
        for (int i = 0; i < numSamples; ++i)
            dst[i] = (WDL_FFT_REAL) src[i];
        wdlInputPtrs[c] = wdlInputData[c].data();
    }

    engine.Add (wdlInputPtrs.data(), numSamples, numChannels);

    int avail = engine.Avail (numSamples);
    WDL_FFT_REAL** out = engine.Get();
    int toCopy = juce::jmin (avail, numSamples);

    for (int c = 0; c < numChannels; ++c)
    {
        auto* dst = buffer.getWritePointer (c);
        
        // If the engine has output for this channel
        if (c < impulseBuffer.GetNumChannels())
        {
            auto* src = out[c];
            for (int i = 0; i < toCopy; ++i)
                dst[i] = (float) src[i];
        }
        else
        {
            juce::FloatVectorOperations::clear (dst, toCopy);
        }

        if (toCopy < numSamples)
            juce::FloatVectorOperations::clear (dst + toCopy, numSamples - toCopy);
    }

    engine.Advance (toCopy);
}

void ConvolReverb::setImpulseList (const std::vector<juce::String>& names, const std::vector<juce::String>& resources)
{
    irNames = names;
    irResources = resources;
    if (!irResources.empty())
        selectImpulse (0); // Select the first one by default
}

void ConvolReverb::selectImpulse (int index)
{
    if (index < 0 || index >= (int)irResources.size())
        return;

    if (currentIndex != index)
    {
        currentIndex = index;
        loadResource (irResources[currentIndex]);
        updateModifiedIR();
    }
}

void ConvolReverb::setLengthRatio (float ratio)
{
    if (currentLengthRatio != ratio)
    {
        currentLengthRatio = ratio;
        updateModifiedIR();
    }
}

void ConvolReverb::setShapeType (int type)
{
    if (currentShapeType != type)
    {
        currentShapeType = type;
        updateModifiedIR();
    }
}

void ConvolReverb::assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
{
    irParam = apvts.getRawParameterValue (prefix + "_IR");
    lengthParam = apvts.getRawParameterValue (prefix + "_Length");
    shapeParam = apvts.getRawParameterValue (prefix + "_Shape");
}

void ConvolReverb::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix)
{
    // IR selection: range depends on the number of loaded IRs, but we need a fixed range for APVTS
    // Max 100 IRs for parameter definition, UI will limit to actual count.
    params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { prefix + "_IR", 1 }, prefix + " IR", 0, 100, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { prefix + "_Length", 1 }, prefix + " Length", 0.0f, 1.0f, 1.0f));
    
    juce::StringArray shapes;
    shapes.add ("Fast Exp");
    shapes.add ("Linear");
    shapes.add ("Slow Log");
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { prefix + "_Shape", 1 }, prefix + " Shape", shapes, 0));
}

void ConvolReverb::loadResource (const juce::String& resourceName)
{
    int dataSize = 0;
    const char* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), dataSize);
    
    if (data == nullptr)
    {
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            if (resourceName.equalsIgnoreCase (BinaryData::namedResourceList[i]))
                { data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], dataSize); break; }
        }
    }

    if (data != nullptr)
    {
        auto* stream = new juce::MemoryInputStream (data, (size_t)dataSize, false);
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatReader> reader (wavFormat.createReaderFor (stream, true));

        if (reader)
        {
            originalIR.setSize ((int)reader->numChannels, (int)reader->lengthInSamples);
            reader->read (&originalIR, 0, (int)reader->lengthInSamples, 0, true, true);
        }
        else
        {
            originalIR.clear(); // Clear if loading fails
        }
    }
    else
    {
        originalIR.clear(); // Clear if resource not found
    }
}

void ConvolReverb::updateModifiedIR()
{
    if (originalIR.getNumSamples() == 0)
    {
        modifiedIR.clear();
        loadImpulseToEngine (modifiedIR); // Load empty IR
        return;
    }

    int newLength = (int)(originalIR.getNumSamples() * currentLengthRatio);
    if (newLength < 16) newLength = 16; // Minimum safety length
    if (newLength > originalIR.getNumSamples()) newLength = originalIR.getNumSamples();

    modifiedIR.makeCopyOf (originalIR);
    modifiedIR.setSize (modifiedIR.getNumChannels(), newLength, true, true); // Resize and clear new part

    // Apply Envelope
    // Shape 0: Exp, 1: Lin, 2: Log
    
    for (int ch = 0; ch < modifiedIR.getNumChannels(); ++ch)
    {
        auto* data = modifiedIR.getWritePointer (ch);
        for (int i = 0; i < newLength; ++i)
        {
            float normPos = (float)i / (float)newLength;
            float gain = 1.0f;

            if (currentShapeType == 0) // Fast Exp
                gain = std::pow (1.0f - normPos, 2.0f);
            else if (currentShapeType == 1) // Linear
                gain = 1.0f - normPos;
            else if (currentShapeType == 2) // Slow Log
                gain = std::sqrt (1.0f - normPos); // Simple approximation

            data[i] *= gain;
        }
        
        // Fade out last few samples to avoid clicks
        int fadeLen = std::min(100, newLength);
        if (newLength > fadeLen)
            modifiedIR.applyGainRamp (ch, newLength - fadeLen, fadeLen, 1.0f, 0.0f);
    }

    loadImpulseToEngine (modifiedIR);
}

void ConvolReverb::loadImpulseToEngine (const juce::AudioBuffer<float>& buffer)
{
    juce::ScopedLock sl (lock);

    int nch = buffer.getNumChannels();
    int len = buffer.getNumSamples();

    // Safety check: WDL_ImpulseBuffer crashes if initialized with 0 channels/length
    if (nch == 0 || len == 0)
    {
        // Load a dummy silent impulse
        nch = 1;
        len = 1;
        impulseBuffer.SetNumChannels (nch);
        impulseBuffer.SetLength (len);
        impulseBuffer.samplerate = currentSampleRate;
        auto* dest = impulseBuffer.impulses[0].Get();
        dest[0] = 0.0;
    }
    else
    {
        impulseBuffer.SetNumChannels (nch);
        impulseBuffer.SetLength (len);
        impulseBuffer.samplerate = currentSampleRate;

        for (int c = 0; c < nch; ++c)
        {
            auto* dest = impulseBuffer.impulses[c].Get();
            auto* src = buffer.getReadPointer (c);
            for (int i = 0; i < len; ++i)
                dest[i] = (WDL_FFT_REAL) src[i];
        }
    }

    engine.SetImpulse (&impulseBuffer);
}

void ConvolReverb::checkParameters()
{
    if (irParam && (int)*irParam != lastIR)
    {
        selectImpulse ((int)*irParam);
        lastIR = (int)*irParam;
    }

    if (lengthParam && *lengthParam != lastLengthRatio)
    {
        setLengthRatio (*lengthParam);
        lastLengthRatio = *lengthParam;
    }

    if (shapeParam && (int)*shapeParam != lastShapeType)
    {
        setShapeType ((int)*shapeParam);
        lastShapeType = (int)*shapeParam;
    }
}
