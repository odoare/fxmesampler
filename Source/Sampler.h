/*
  ==============================================================================

    Sampler.h
    Created: 30 Jan 2026 3:56:29pm
    Author:  doare

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

//==============================================================================
struct Sound
{
    juce::String name;
    const char* data = nullptr;
    int dataSize = 0;
    juce::Range<int> midiNoteRange { 0, 127 };
    int basePitch = 60;
    int muteGroup = 0;
    bool isOneShot = true;
    juce::Range<int> velocityRange { 0, 128 };
    std::vector<int> outputChannels;
    double attack = 0.001;  // Seconds
    double decay = 0.0;     // Seconds
    double sustain = 1.0;   // 0.0 to 1.0
    double release = 0.1;   // Seconds

    // Internal audio data
    juce::AudioBuffer<float> audioBuffer;
    double sourceSampleRate = 44100.0;

    void load (juce::AudioFormatManager& formatManager);
};

//==============================================================================
class Voice
{
public:
    Voice() = default;

    void start (const Sound* sound, int note, float velocity, double sampleRate);
    void stop();
    void choke();
    void noteOff();
    bool isActive() const;
    const Sound* getSound() const { return activeSound; }
    int getNote() const { return currentNote; }
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples);

private:
    enum class State { Attack, Decay, Sustain, Release, Idle };
    State state = State::Idle;

    const Sound* activeSound = nullptr;
    int currentNote = 0;
    double currentPosition = 0.0;
    double increment = 1.0;
    double currentSampleRate = 44100.0;
    float currentVelocity = 0.0f;
    float envelopeVal = 0.0f;
    double attackRate = 0.0;
    double decayRate = 0.0;
    double sustainLevel = 0.0;
    double releaseRate = 0.0;
};

//==============================================================================
class Sampler
{
public:
    Sampler();
    ~Sampler();

    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void addSound (const Sound& sound);
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void loadSamplesFromXml (const void* xmlData, int xmlSize);
    
    int getNumOutputChannels() const { return numOutputChannels; }

private:
    std::vector<Sound> sounds;
    std::vector<std::unique_ptr<Voice>> voices;
    static const int maxVoices = 32;
    double currentSampleRate = 44100.0;
    juce::AudioFormatManager formatManager;
    
    int numOutputChannels = 2;

    Voice* findFreeVoice();
};
