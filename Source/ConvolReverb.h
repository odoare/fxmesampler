/*
  ==============================================================================

    ConvolReverb.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../WDL/WDL/convoengine.h"
#include <atomic>
#include <vector>

class ConvolReverb
{
public:
    ConvolReverb();
    ~ConvolReverb();

    void prepare (double sampleRate, int samplesPerBlock);
    void process (juce::AudioBuffer<float>& buffer);

    void setImpulseList (const std::vector<juce::String>& names, const std::vector<juce::String>& resourceNames);
    void selectImpulse (int index);
    void setLengthRatio (float ratio); // 0.0 to 1.0
    void setShapeType (int type); // 0: Exp, 1: Lin, 2: Log

    const std::vector<juce::String>& getImpulseNames() const { return irNames; }
    const juce::AudioBuffer<float>& getModifiedIR() const { return modifiedIR; }
    int getCurrentImpulseIndex() const { return currentIndex; }
    float getCurrentLengthRatio() const { return currentLengthRatio; }
    int getCurrentShapeType() const { return currentShapeType; }

    // APVTS integration
    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix);

private:
    // WDL Engine
    WDL_ImpulseBuffer impulseBuffer;
    WDL_ConvolutionEngine_Div engine;
    juce::CriticalSection lock;
    std::vector<std::vector<WDL_FFT_REAL>> wdlInputData;
    std::vector<WDL_FFT_REAL*> wdlInputPtrs;

    double currentSampleRate = 44100.0;

    std::vector<juce::String> irNames;
    std::vector<juce::String> irResources;
    juce::AudioBuffer<float> originalIR; // Stores the raw loaded IR
    juce::AudioBuffer<float> modifiedIR; // Stores the IR after length/shape modifications

    int currentIndex = -1;
    float currentLengthRatio = 1.0f;
    int currentShapeType = 0;

    // Parameter pointers
    std::atomic<float>* irParam = nullptr;
    std::atomic<float>* lengthParam = nullptr;
    std::atomic<float>* shapeParam = nullptr;

    // Last values for change detection
    int lastIR = -1;
    float lastLengthRatio = -1.0f;
    int lastShapeType = -1;

    void loadResource (const juce::String& resourceName);
    void updateModifiedIR(); // Applies length/shape to originalIR and loads into wdlReverb
    void loadImpulseToEngine (const juce::AudioBuffer<float>& buffer);
    void checkParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConvolReverb)
};