/*
  ==============================================================================

    MixerStrips.h

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
/**
 * @struct Send
 * @brief Represents an audio send to a bus.
 */
struct Send
{
    juce::String busName;
    BusStrip* bus = nullptr;
    std::atomic<float>* gainParam = nullptr;
    std::atomic<float>* prePostParam = nullptr; // 0.0 = Post, 1.0 = Pre
    float currentGain = 0.0f;
};

//==============================================================================
/**
 * @class MixerStrip
 * @brief Base class for a mixer strip.
 */
class MixerStrip
{
public:
    /**
     * @brief Constructor.
     * @param name The name of the strip.
     */
    MixerStrip (const juce::String& name) : name (name) {}

    /**
     * @brief Destructor.
     */
    virtual ~MixerStrip() = default;

    /**
     * @brief Prepares the strip for playback.
     * @param sampleRate The sample rate.
     * @param samplesPerBlock The expected number of samples per block.
     */
    virtual void prepare (double sampleRate, int samplesPerBlock) = 0;

    /**
     * @brief Processes a block of audio.
     * @param input The input buffer containing all channels.
     * @param mixBuffer The mix bus buffer to sum into.
     * @param outputBuffer The main output buffer (for direct routing).
     * @param inputChannelOffset The starting channel index in the input buffer for this strip.
     */
    virtual void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset) = 0;

    /**
     * @brief Returns the number of input channels this strip consumes.
     * @return Number of channels.
     */
    virtual int getNumInputChannels() const = 0;

    /**
     * @brief Assigns APVTS parameters to internal pointers.
     * @param apvts The AudioProcessorValueTreeState.
     */
    virtual void assignParameters (juce::AudioProcessorValueTreeState& apvts) = 0;

    /**
     * @brief Adds parameters to the layout builder.
     * @param params Vector to add parameters to.
     */
    virtual void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) = 0;

    /** @brief Gets the name of the strip. */
    juce::String getName() const { return name; }

    /** @brief Gets the effect chain associated with this strip. */
    EffectChain* getEffectChain() const { return effectChain.get(); }

    /** @brief Sets the effect chain for this strip. */
    void setEffectChain (std::unique_ptr<EffectChain> chain) { effectChain = std::move (chain); }

    /** @brief Checks if the strip is muted. */
    bool isMute() const { return muteParam && *muteParam > 0.5f; }

    /** @brief Checks if the strip is soloed. */
    bool isSolo() const { return soloParam && *soloParam > 0.5f; }

    /** @brief Checks if the strip can be soloed. */
    virtual bool isSoloable() const { return true; }

    /** @brief Sets the strip icon image. */
    void setImage (const juce::Image& img) { image = img; }

    /** @brief Gets the strip icon image. */
    juce::Image getImage() const { return image; }

    /** @brief Sets the strip color. */
    void setColor (juce::Colour c) { color = c; }

    /** @brief Gets the strip color. */
    juce::Colour getColor() const { return color; }

    /**
     * @brief Adds a send to a bus.
     * @param busName Name of the destination bus.
     * @param bus Pointer to the BusStrip.
     */
    void addSend (const juce::String& busName, BusStrip* bus);

    /**
     * @brief Processes sends for the current block.
     * @param buffer The audio buffer to send from.
     * @param isPre True for pre-fader sends, false for post-fader.
     */
    void processSends (juce::AudioBuffer<float>& buffer, bool isPre);

    /** @brief Gets the list of sends. */
    const std::vector<Send>& getSends() const { return sends; }

    /** @brief Processes the effect chain. */
    void processEffects (juce::AudioBuffer<float>& buffer);

    /** @brief Clears meter values (used when strip is not processing). */
    void clearMeters();

    /** @brief Sets the BPM for time-based effects. */
    void setBPM(double bpm);

    VuMeter meterL, meterR;

protected:
    juce::String name;
    std::unique_ptr<EffectChain> effectChain;
    juce::AudioBuffer<float> tempBuffer;
    std::atomic<float>* muteParam = nullptr;
    std::atomic<float>* soloParam = nullptr;
    juce::Image image;
    juce::Colour color { 0 };
    std::vector<Send> sends;
    std::vector<std::atomic<float>*> routeParams; // 0: Main, 1: Aux1, 2: Aux2, 3: Aux3
};

/**
 * @class AmbisonicStrip
 * @brief Mixer strip for Ambisonic B-format (1st order) input.
 */
class AmbisonicStrip : public MixerStrip
{
public:
    AmbisonicStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 4; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;
    // void clearMeters() override;

    AmbixToMS ambix;

    std::atomic<float>* azParam = nullptr;
    std::atomic<float>* elParam = nullptr;
    std::atomic<float>* wParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
};

/**
 * @class AmbisonicMonoStrip
 * @brief Mixer strip combining an Ambisonic input and a Mono input.
 */
class AmbisonicMonoStrip : public MixerStrip
{
public:
    AmbisonicMonoStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 5; } // 4 Ambisonic + 1 Mono
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;

    AmbixToMS ambix;
    float pan = 0.0f; // For the mono part
    float mix = 0.0f; // -1.0 (Ambix) to 1.0 (Mono)

    std::atomic<float>* azParam = nullptr;
    std::atomic<float>* elParam = nullptr;
    std::atomic<float>* wParam = nullptr;
    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
};

/**
 * @class MSStrip
 * @brief Mixer strip for Mid-Side stereo input.
 */
class MSStrip : public MixerStrip
{
public:
    MSStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 2; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;
    // void clearMeters() override;

    float pan = 0.0f;
    float width = 1.0f;
    float level = 1.0f;
    VuMeter meterL, meterR;

    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* wParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
};

/**
 * @class StereoStrip
 * @brief Mixer strip for standard stereo input.
 */
class StereoStrip : public MixerStrip
{
public:
    StereoStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 2; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;
    // void clearMeters() override;

    float pan = 0.0f;
    float width = 1.0f;
    float level = 1.0f;
    // VuMeter meterL, meterR;

    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* wParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
};

/**
 * @class MonoStrip
 * @brief Mixer strip for mono input.
 */
class MonoStrip : public MixerStrip
{
public:
    MonoStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 1; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;
    // void clearMeters() override;

    float pan = 0.0f;
    float level = 1.0f;
    // VuMeter meterL, meterR;

    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
};

/**
 * @class StereoReverbStrip
 * @brief Mixer strip with built-in stereo convolution reverb.
 */
class StereoReverbStrip : public MixerStrip
{
public:
    StereoReverbStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 1; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;
    // void clearMeters() override;

    /**
     * @brief Sets the list of available impulse responses.
     * @param names List of display names.
     * @param resources List of resource identifiers.
     */
    void setImpulseList (const juce::StringArray& names, const juce::StringArray& resources);

    ConvolReverb reverb;
    float pan = 0.0f;
    float level = 1.0f;
    // VuMeter meterL, meterR;

    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
    std::atomic<float>* irParam = nullptr; // For dropdown selection
};

/**
 * @class MonoReverbStrip
 * @brief Mixer strip with built-in mono convolution reverb.
 */
class MonoReverbStrip : public MixerStrip
{
public:
    MonoReverbStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 1; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;
    // void clearMeters() override;

    /**
     * @brief Sets the list of available impulse responses.
     * @param names List of display names.
     * @param resources List of resource identifiers.
     */
    void setImpulseList (const juce::StringArray& names, const juce::StringArray& resources);

    ConvolReverb reverb;
    float pan = 0.0f;
    float level = 1.0f;
    // VuMeter meterL, meterR;

    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
    std::atomic<float>* irParam = nullptr; // For dropdown selection
};

/**
 * @class MasterStrip
 * @brief The master output strip.
 */
class MasterStrip : public MixerStrip
{
public:
    MasterStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 2; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;
    bool isSoloable() const override { return false; }
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;
    // void clearMeters() override;

    float pan = 0.0f;
    float width = 1.0f;
    float level = 1.0f;
    // VuMeter meterL, meterR;

    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* wParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
};

/**
 * @class BusStrip
 * @brief A bus strip that sums inputs from sends.
 */
class BusStrip : public MixerStrip
{
public:
    BusStrip (const juce::String& name);
    void prepare (double sampleRate, int samplesPerBlock) override;
    void process (const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& mixBuffer, juce::AudioBuffer<float>& outputBuffer, int inputChannelOffset) override;
    int getNumInputChannels() const override { return 0; }
    void assignParameters (juce::AudioProcessorValueTreeState& apvts) override;
    void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params) override;
    // void clearMeters() override;

    /**
     * @brief Adds input to the bus buffer.
     * @param source The source audio buffer.
     * @param gain The gain to apply.
     */
    void addInput (const juce::AudioBuffer<float>& source, float gain);

    /** @brief Clears the bus buffer before processing. */
    void clearBusBuffer();

    // VuMeter meterL, meterR;

private:
    juce::AudioBuffer<float> busBuffer;
    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* wParam = nullptr;
    std::atomic<float>* lvlParam = nullptr;
    float pan = 0.0f;
    float width = 1.0f;
    float level = 1.0f;
};