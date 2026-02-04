/*
  ==============================================================================

    Mixer.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "AmbixToMS.h"
#include "VuMeter.h"
#include "Equalizer.h"
#include "Compressor.h"
#include "Tube.h"
#include <atomic>

//==============================================================================
class MixerStrip
{
public:
    MixerStrip (const juce::String& name) : name (name) {}
    virtual ~MixerStrip() = default;

    virtual void prepare (double sampleRate, int samplesPerBlock) = 0;
    virtual void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset) = 0;
    virtual int getNumInputChannels() const = 0;

    virtual void assignParameters (juce::AudioProcessorValueTreeState& apvts) = 0;
    juce::String getName() const { return name; }
    Equalizer& getEQ() { return eq; }
    Compressor& getComp() { return comp; }
    Tube& getTube() { return tube; }

    bool isMute() const { return muteParam && *muteParam > 0.5f; }
    bool isSolo() const { return soloParam && *soloParam > 0.5f; }

    void setImage (const juce::Image& img) { image = img; }
    juce::Image getImage() const { return image; }

    void processEffects (juce::AudioBuffer<float>& buffer);

protected:
    juce::String name;
    Equalizer eq;
    Compressor comp;
    Tube tube;
    juce::AudioBuffer<float> tempBuffer;
    std::atomic<float>* muteParam = nullptr;
    std::atomic<float>* soloParam = nullptr;
    std::atomic<float>* orderParam = nullptr;
    juce::Image image;
};

class AmbisonicStrip : public MixerStrip
{
public:
    AmbisonicStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 4; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;

    AmbixToMS ambix;
    VuMeter meterL, meterR;

    std::atomic<float>* azParam = nullptr;
    std::atomic<float>* elParam = nullptr;
    std::atomic<float>* wParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
};

class MSStrip : public MixerStrip
{
public:
    MSStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 2; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;

    float pan = 0.0f;
    float width = 1.0f;
    float level = 1.0f;
    VuMeter meterL, meterR;

    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* wParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
};

class StereoStrip : public MixerStrip
{
public:
    StereoStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 2; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;

    float pan = 0.0f;
    float width = 1.0f;
    float level = 1.0f;
    VuMeter meterL, meterR;

    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* wParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
};

class MonoStrip : public MixerStrip
{
public:
    MonoStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 1; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;

    float pan = 0.0f;
    float level = 1.0f;
    VuMeter meter;

    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
};

//==============================================================================
class Mixer
{
public:
    Mixer() = default;

    void prepare (double sampleRate, int samplesPerBlock);
    void processBlock (const juce::AudioBuffer<float>& inputBuffer, juce::AudioBuffer<float>& outputBuffer);
    void loadFromXml (const void* xmlData, int xmlSize);
    void assignParameters (juce::AudioProcessorValueTreeState& apvts);

    const std::vector<std::unique_ptr<MixerStrip>>& getStrips() const { return strips; }

private:
    std::vector<std::unique_ptr<MixerStrip>> strips;
    double currentSampleRate = 44100.0;
    int currentSamplesPerBlock = 512;
};