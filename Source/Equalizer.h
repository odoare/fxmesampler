/*
  ==============================================================================

    Equalizer.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <atomic>

/**
 * @class Equalizer
 * @brief A 4-band equalizer with Low Shelf, two Peaking bands, and High Shelf.
 *
 * This class processes audio using a set of biquad filters per channel.
 */
class Equalizer
{
public:
    /**
     * @brief Constructor.
     */
    Equalizer();

    /**
     * @brief Prepares the equalizer for playback.
     * @param sampleRate The sample rate of the audio stream.
     * @param numChannels The number of audio channels to process.
     */
    void prepare (double sampleRate, int numChannels);

    /**
     * @brief Processes a block of audio samples.
     * @param buffer The audio buffer to process in-place.
     */
    void process (juce::AudioBuffer<float>& buffer);

    /**
     * @brief Sets parameters for Band 0 (Low Shelf).
     * @param freq The corner frequency in Hz.
     * @param gaindB The gain in decibels.
     */
    void setLowShelf (float freq, float gaindB);

    /**
     * @brief Sets parameters for Band 1 (Peaking).
     * @param freq The center frequency in Hz.
     * @param Q The quality factor.
     * @param gaindB The gain in decibels.
     */
    void setBand1 (float freq, float Q, float gaindB);

    /**
     * @brief Sets parameters for Band 2 (Peaking).
     * @param freq The center frequency in Hz.
     * @param Q The quality factor.
     * @param gaindB The gain in decibels.
     */
    void setBand2 (float freq, float Q, float gaindB);

    /**
     * @brief Sets parameters for Band 3 (Peaking).
     * @param freq The center frequency in Hz.
     * @param Q The quality factor.
     * @param gaindB The gain in decibels.
     */
    void setBand3 (float freq, float Q, float gaindB);

    /**
     * @brief Sets parameters for Band 3 (High Shelf).
     * @param freq The corner frequency in Hz.
     * @param gaindB The gain in decibels.
     */
    void setHighShelf (float freq, float gaindB);

    /**
     * @brief Enables or disables the equalizer.
     * @param shouldBeOn True to enable, false to bypass.
     */
    void setOn (bool shouldBeOn);

    /**
     * @brief Checks if the equalizer is currently enabled.
     * @return True if enabled, false otherwise.
     */
    bool isOn() const;

    /**
     * @brief Assigns parameters from the APVTS to this equalizer.
     * @param apvts The AudioProcessorValueTreeState containing parameters.
     * @param prefix The prefix used for parameter IDs.
     */
    void assignParameters (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);

    /**
     * @brief Checks for parameter updates and applies them.
     */
    void checkParameters();

    /**
     * @brief Adds the equalizer parameters to the provided vector.
     * @param params The vector to add parameters to.
     * @param prefix The prefix used for parameter IDs.
     */
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& prefix);

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
        Biquad band3;
        Biquad highShelf;
    };

    std::vector<ChannelStrip> channels;
    double currentSampleRate = 44100.0;
    bool on = true;
    float postGain = 1.0f;

    // Parameter cache
    struct { float f, g; } lsParams { 100.0f, 0.0f };
    struct { float f, q, g; } b1Params { 500.0f, 1.0f, 0.0f };
    struct { float f, q, g; } b2Params { 2000.0f, 1.0f, 0.0f };
    struct { float f, q, g; } b3Params { 3500.0f, 1.0f, 0.0f };
    struct { float f, g; } hsParams { 5000.0f, 0.0f };

    // Parameter Pointers
    std::atomic<float>* onParam = nullptr;
    std::atomic<float>* postGainParam = nullptr;
    std::atomic<float>* lsFreqParam = nullptr; std::atomic<float>* lsGainParam = nullptr;
    std::atomic<float>* b1FreqParam = nullptr; std::atomic<float>* b1QParam = nullptr; std::atomic<float>* b1GainParam = nullptr;
    std::atomic<float>* b2FreqParam = nullptr; std::atomic<float>* b2QParam = nullptr; std::atomic<float>* b2GainParam = nullptr;
    std::atomic<float>* b3FreqParam = nullptr; std::atomic<float>* b3QParam = nullptr; std::atomic<float>* b3GainParam = nullptr;
    std::atomic<float>* hsFreqParam = nullptr; std::atomic<float>* hsGainParam = nullptr;

    // Last values for change detection
    float lastOn = -1.0f;
    float lastPostGain = -100.0f;
    float lastLsFreq = -1.0f, lastLsGain = -100.0f;
    float lastB1Freq = -1.0f, lastB1Q = -1.0f, lastB1Gain = -100.0f;
    float lastB2Freq = -1.0f, lastB2Q = -1.0f, lastB2Gain = -100.0f;
    float lastB3Freq = -1.0f, lastB3Q = -1.0f, lastB3Gain = -100.0f;
    float lastHsFreq = -1.0f, lastHsGain = -100.0f;

    void updateCoefficients();
    void calcLowShelf (Biquad& bq, float f, float g);
    void calcPeaking (Biquad& bq, float f, float q, float g);
    void calcHighShelf (Biquad& bq, float f, float g);
};