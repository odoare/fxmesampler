/*
  ==============================================================================

    Mixer.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "AmbixToMS.h"
#include "Equalizer.h"
#include "Compressor.h"

class Mixer
{
public:
    Mixer() = default;

    void prepare (double sampleRate, int samplesPerBlock);
    void processBlock (const juce::AudioBuffer<float>& inputBuffer, juce::AudioBuffer<float>& outputBuffer);

    // Ambisonic Controls
    void setAmbisonicAzimuth (float degrees);
    void setAmbisonicElevation (float degrees);
    void setAmbisonicWidth (float width);
    void setAmbisonicLevel (float level);

    // Levels
    void setAmbienceLevel (float level);
    void setKickLevel (float level);
    void setKickPan (float pan);
    void setSnareLevel (float level);
    void setSnarePan (float pan);
    void setHiHatLevel (float level);
    void setHiHatPan (float pan);

    // Effect Accessors
    Equalizer& getAmbisonicEqualizer() { return ambisonicEQ; }
    Compressor& getAmbisonicCompressor() { return ambisonicComp; }
    Equalizer& getAmbienceEqualizer() { return ambienceEQ; }
    Compressor& getAmbienceCompressor() { return ambienceComp; }
    Equalizer& getKickEqualizer() { return kickEQ; }
    Compressor& getKickCompressor() { return kickComp; }
    Equalizer& getSnareEqualizer() { return snareEQ; }
    Compressor& getSnareCompressor() { return snareComp; }
    Equalizer& getHiHatEqualizer() { return hhEQ; }
    Compressor& getHiHatCompressor() { return hhComp; }

private:
    AmbixToMS ambixToMS;
    Equalizer ambisonicEQ, ambienceEQ, kickEQ, snareEQ, hhEQ;
    Compressor ambisonicComp, ambienceComp, kickComp, snareComp, hhComp;
    
    juce::AudioBuffer<float> tempStereoBuffer;
    juce::AudioBuffer<float> tempMonoBuffer;

    float ambienceLevel = 1.0f;

    struct Track { float level = 1.0f; float pan = 0.0f; };
    Track kickTrack, snareTrack, hhTrack;
};