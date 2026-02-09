/*
  ==============================================================================

    Mixer.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "AmbixToMS.h"
#include "VuMeter.h"
#include "EffectChainDynamics.h"
#include "ConvolReverb.h"
#include "EffectChainReverb.h"
#include <atomic>

// Forward declaration
class BusStrip;

//==============================================================================
struct Send
{
    juce::String busName;
    BusStrip* bus = nullptr;
    std::atomic<float>* gainParam = nullptr;
    float currentGain = 0.0f;
};

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
    virtual void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) = 0;
    juce::String getName() const { return name; }
    EffectChain* getEffectChain() const { return effectChain.get(); }
    void setEffectChain (std::unique_ptr<EffectChain> chain) { effectChain = std::move (chain); }

    bool isMute() const { return muteParam && *muteParam > 0.5f; }
    bool isSolo() const { return soloParam && *soloParam > 0.5f; }

    void setImage (const juce::Image& img) { image = img; }
    juce::Image getImage() const { return image; }

    void setColor (juce::Colour c) { color = c; }
    juce::Colour getColor() const { return color; }

    void addSend (const juce::String& busName, BusStrip* bus);
    void processSends (juce::AudioBuffer<float>& buffer);
    const std::vector<Send>& getSends() const { return sends; }

    void processEffects (juce::AudioBuffer<float>& buffer);
    virtual void clearMeters() {}

protected:
    juce::String name;
    std::unique_ptr<EffectChain> effectChain;
    juce::AudioBuffer<float> tempBuffer;
    std::atomic<float>* muteParam = nullptr;
    std::atomic<float>* soloParam = nullptr;
    juce::Image image;
    juce::Colour color { 0 };
    std::vector<Send> sends;
};

class AmbisonicStrip : public MixerStrip
{
public:
    AmbisonicStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 4; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;
    void clearMeters() override;

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
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;
    void clearMeters() override;

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
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;
    void clearMeters() override;

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
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;
    void clearMeters() override;

    float pan = 0.0f;
    float level = 1.0f;
    VuMeter meter;

    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
};

class StereoReverbStrip : public MixerStrip
{
public:
    StereoReverbStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 1; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;
    void clearMeters() override;
    void setImpulseList (const std::vector<juce::String>& names, const std::vector<juce::String>& resources);

    ConvolReverb reverb;
    float pan = 0.0f;
    float level = 1.0f;
    VuMeter meterL, meterR;

    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
    std::atomic<float>* irParam = nullptr; // For dropdown selection
};

class MonoReverbStrip : public MixerStrip
{
public:
    MonoReverbStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 1; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;
    void clearMeters() override;
    void setImpulseList (const std::vector<juce::String>& names, const std::vector<juce::String>& resources);

    ConvolReverb reverb;
    float pan = 0.0f;
    float level = 1.0f;
    VuMeter meter;

    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
    std::atomic<float>* irParam = nullptr; // For dropdown selection
};

class MasterStrip : public MixerStrip
{
public:
    MasterStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 2; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;
    void clearMeters() override;

    float pan = 0.0f;
    float width = 1.0f;
    float level = 1.0f;
    VuMeter meterL, meterR;

    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* wParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
};

class BusStrip : public MixerStrip
{
public:
    BusStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 0; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;
    void clearMeters() override;

    void addInput (const juce::AudioBuffer<float>& source, float gain);
    void clearBusBuffer();

    VuMeter meterL, meterR;

private:
    juce::AudioBuffer<float> busBuffer;
    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* wParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
    float pan = 0.0f;
    float width = 1.0f;
    float level = 1.0f;
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
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params);

    const std::vector<std::unique_ptr<MixerStrip>>& getStrips() const { return strips; }
    MasterStrip& getMasterStrip() { return masterStrip; }

private:
    std::vector<std::unique_ptr<MixerStrip>> strips;
    MasterStrip masterStrip { "Master" };
    juce::AudioBuffer<float> mixBuffer;
    double currentSampleRate = 44100.0;
    int currentSamplesPerBlock = 512;
    juce::CriticalSection lock;
};