/*
  ==============================================================================

    ConvolReverb.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../WDL/WDL/convoengine.h"

class ConvolReverb
{
public:
    ConvolReverb();
    ~ConvolReverb();

    void prepare (double sampleRate, int samplesPerBlock);
    void process (juce::AudioBuffer<float>& buffer);

    void loadImpulse (const juce::File& file);
    void loadImpulse (const void* data, size_t size);

private:
    void loadImpulseFromBuffer (juce::AudioBuffer<float>& buffer, double fileSampleRate);

    WDL_ImpulseBuffer impulseBuffer;
    WDL_ConvolutionEngine_Div engine;

    double currentSampleRate = 44100.0;
    juce::CriticalSection lock;

    // Temporary buffers for WDL conversion (float <-> WDL_FFT_REAL)
    std::vector<std::vector<WDL_FFT_REAL>> wdlInputData;
    std::vector<WDL_FFT_REAL*> wdlInputPtrs;
};