/*
  ==============================================================================

    Mixer.cpp

  ==============================================================================
*/

#include "Mixer.h"

void Mixer::prepare (double sampleRate, int samplesPerBlock)
{
    ambisonicEQ.prepare (sampleRate, 2);
    ambisonicComp.prepare (sampleRate, 2);
    
    ambienceEQ.prepare (sampleRate, 1);
    ambienceComp.prepare (sampleRate, 1);
    kickEQ.prepare (sampleRate, 1);
    kickComp.prepare (sampleRate, 1);
    snareEQ.prepare (sampleRate, 1);
    snareComp.prepare (sampleRate, 1);
    hhEQ.prepare (sampleRate, 1);
    hhComp.prepare (sampleRate, 1);

    tempStereoBuffer.setSize (2, samplesPerBlock);
    tempMonoBuffer.setSize (1, samplesPerBlock);
}

void Mixer::setAmbisonicAzimuth (float degrees)   { ambixToMS.setAzimuth (degrees); }
void Mixer::setAmbisonicElevation (float degrees) { ambixToMS.setElevation (degrees); }
void Mixer::setAmbisonicWidth (float w)           { ambixToMS.setWidth (w); }
void Mixer::setAmbisonicLevel (float level)       { ambixToMS.setLevel (level); }

void Mixer::setAmbienceLevel (float level)        { ambienceLevel = level; }

void Mixer::setKickLevel (float level)            { kickTrack.level = level; }
void Mixer::setKickPan (float pan)                { kickTrack.pan = pan; }
void Mixer::setSnareLevel (float level)           { snareTrack.level = level; }
void Mixer::setSnarePan (float pan)               { snareTrack.pan = pan; }
void Mixer::setHiHatLevel (float level)           { hhTrack.level = level; }
void Mixer::setHiHatPan (float pan)               { hhTrack.pan = pan; }

void Mixer::processBlock (const juce::AudioBuffer<float>& inputBuffer, juce::AudioBuffer<float>& outputBuffer)
{
    // Ensure we have enough inputs
    if (inputBuffer.getNumChannels() < 8)
        return;

    int numSamples = outputBuffer.getNumSamples();
    
    // --- Ambisonics (Stereo) ---
    tempStereoBuffer.clear();
    // AmbixToMS adds to the output buffer, so we use temp buffer to capture, process, then mix
    ambixToMS.process (inputBuffer, tempStereoBuffer);
    ambisonicEQ.process (tempStereoBuffer);
    ambisonicComp.process (tempStereoBuffer);
    
    for (int ch = 0; ch < 2; ++ch)
        outputBuffer.addFrom (ch, 0, tempStereoBuffer, ch, 0, numSamples);

    // --- Mono Tracks Helper ---
    auto processMonoTrack = [&](int inputCh, Equalizer& eq, Compressor& comp, float level, float pan)
    {
        tempMonoBuffer.copyFrom (0, 0, inputBuffer, inputCh, 0, numSamples);
        
        eq.process (tempMonoBuffer);
        comp.process (tempMonoBuffer);
        
        float gainL = level * 0.5f * (1.0f - pan);
        float gainR = level * 0.5f * (1.0f + pan);
        
        outputBuffer.addFrom (0, 0, tempMonoBuffer, 0, 0, numSamples, gainL);
        outputBuffer.addFrom (1, 0, tempMonoBuffer, 0, 0, numSamples, gainR);
    };

    // Ambience (Ch 4)
    processMonoTrack (4, ambienceEQ, ambienceComp, ambienceLevel, 0.0f);
    // Kick (Ch 5)
    processMonoTrack (5, kickEQ, kickComp, kickTrack.level, kickTrack.pan);
    // Snare (Ch 6)
    processMonoTrack (6, snareEQ, snareComp, snareTrack.level, snareTrack.pan);
    // HH (Ch 7)
    processMonoTrack (7, hhEQ, hhComp, hhTrack.level, hhTrack.pan);
}