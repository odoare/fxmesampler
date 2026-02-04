/*
  ==============================================================================

    Equalizer.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <atomic>

class Equalizer
{
public:
    Equalizer();

    void prepare (double sampleRate, int numChannels);
    void process (juce::AudioBuffer<float>& buffer);

    // Band 0: Low Shelf
    void setLowShelf (float freq, float gaindB);
    // Band 1: Peaking
    void setBand1 (float freq, float Q, float gaindB);
    // Band 2: Peaking
    void setBand2 (float freq, float Q, float gaindB);
    // Band 3: High Shelf
    void setHighShelf (float freq, float gaindB);

    void setOn (bool shouldBeOn);
    bool isOn() const;

    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    void checkParameters();

private:
    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;

        void reset() { z1 = 0.0f; z2 = 0.0f; }
        float process (float in);
    };

    struct ChannelStrip
    {
        Biquad lowShelf;
        Biquad band1;
        Biquad band2;
        Biquad highShelf;
    };

    std::vector<ChannelStrip> channels;
    double currentSampleRate = 44100.0;
    bool on = true;
    float preGain = 1.0f;
    float postGain = 1.0f;

    // Parameter cache
    struct { float f, g; } lsParams { 100.0f, 0.0f };
    struct { float f, q, g; } b1Params { 500.0f, 1.0f, 0.0f };
    struct { float f, q, g; } b2Params { 2000.0f, 1.0f, 0.0f };
    struct { float f, g; } hsParams { 5000.0f, 0.0f };

    // Parameter Pointers
    std::atomic<float>* onParam = nullptr;
    std::atomic<float>* preGainParam = nullptr;
    std::atomic<float>* postGainParam = nullptr;
    std::atomic<float>* lsFreqParam = nullptr; std::atomic<float>* lsGainParam = nullptr;
    std::atomic<float>* b1FreqParam = nullptr; std::atomic<float>* b1QParam = nullptr; std::atomic<float>* b1GainParam = nullptr;
    std::atomic<float>* b2FreqParam = nullptr; std::atomic<float>* b2QParam = nullptr; std::atomic<float>* b2GainParam = nullptr;
    std::atomic<float>* hsFreqParam = nullptr; std::atomic<float>* hsGainParam = nullptr;

    // Last values for change detection
    float lastOn = -1.0f;
    float lastPreGain = -100.0f;
    float lastPostGain = -100.0f;
    float lastLsFreq = -1.0f, lastLsGain = -100.0f;
    float lastB1Freq = -1.0f, lastB1Q = -1.0f, lastB1Gain = -100.0f;
    float lastB2Freq = -1.0f, lastB2Q = -1.0f, lastB2Gain = -100.0f;
    float lastHsFreq = -1.0f, lastHsGain = -100.0f;

    void updateCoefficients();
    void calcLowShelf (Biquad& bq, float f, float g);
    void calcPeaking (Biquad& bq, float f, float q, float g);
    void calcHighShelf (Biquad& bq, float f, float g);
};