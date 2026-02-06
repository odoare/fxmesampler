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

void ConvolReverb::loadImpulse (const juce::File& file)
{
    juce::AudioFormatManager manager;
    manager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (manager.createReaderFor (file));
    if (reader)
    {
        juce::AudioBuffer<float> buffer (reader->numChannels, (int) reader->lengthInSamples);
        reader->read (&buffer, 0, (int) reader->lengthInSamples, 0, true, true);
        loadImpulseFromBuffer (buffer, reader->sampleRate);
    }
}

void ConvolReverb::loadImpulse (const void* data, size_t size)
{
    juce::AudioFormatManager manager;
    manager.registerBasicFormats();

    auto stream = std::make_unique<juce::MemoryInputStream> (data, size, false);
    std::unique_ptr<juce::AudioFormatReader> reader (manager.createReaderFor (std::move (stream)));

    if (reader)
    {
        juce::AudioBuffer<float> buffer (reader->numChannels, (int) reader->lengthInSamples);
        reader->read (&buffer, 0, (int) reader->lengthInSamples, 0, true, true);
        loadImpulseFromBuffer (buffer, reader->sampleRate);
    }
}

void ConvolReverb::loadImpulseFromBuffer (juce::AudioBuffer<float>& buffer, double fileSampleRate)
{
    juce::ScopedLock sl (lock);

    // Note: Resampling is skipped for simplicity. In a production environment, 
    // you should resample 'buffer' from fileSampleRate to currentSampleRate here.

    int nch = buffer.getNumChannels();
    int len = buffer.getNumSamples();

    impulseBuffer.impulses.list.Empty (true);
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

    engine.SetImpulse (&impulseBuffer);
}

void ConvolReverb::process (juce::AudioBuffer<float>& buffer)
{
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