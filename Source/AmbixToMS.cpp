/*
  ==============================================================================

    AmbixToMS.cpp

  ==============================================================================
*/

#include "AmbixToMS.h"

void AmbixToMS::setAzimuth (float degrees)   { azimuth = degrees; }
void AmbixToMS::setElevation (float degrees) { elevation = degrees; }
void AmbixToMS::setWidth (float w)           { width = w; }
void AmbixToMS::setLevel (float l)           { level = l; }

void AmbixToMS::process (const juce::AudioBuffer<float>& inputBuffer, juce::AudioBuffer<float>& outputBuffer)
{
    if (inputBuffer.getNumChannels() < 4 || outputBuffer.getNumChannels() < 2)
        return;

    int numSamples = outputBuffer.getNumSamples();

    // Precompute Ambisonic coefficients
    float azRad = juce::degreesToRadians (azimuth);
    float elRad = juce::degreesToRadians (elevation);

    float ca = std::cos (azRad);
    float sa = std::sin (azRad);
    float ce = std::cos (elRad);
    float se = std::sin (elRad);

    // Pointers to input channels
    // ACN: 0=W, 1=Y, 2=Z, 3=X
    const float* wCh = inputBuffer.getReadPointer (0);
    const float* yCh = inputBuffer.getReadPointer (1);
    const float* zCh = inputBuffer.getReadPointer (2);
    const float* xCh = inputBuffer.getReadPointer (3);

    // Pointers to output channels (Stereo)
    float* outL = outputBuffer.getWritePointer (0);
    float* outR = outputBuffer.getWritePointer (1);

    for (int i = 0; i < numSamples; ++i)
    {
        float w = wCh[i];
        float y = yCh[i];
        float z = zCh[i];
        float x = xCh[i];

        // Virtual MS Decoding
        // Mid (Cardioid): 0.5 * (W + V . u)
        // V = (X, Y, Z), u = (cosA cosE, sinA cosE, sinE)
        float mid = 0.5f * (w + x * ca * ce + y * sa * ce + z * se);

        // Side (Figure-8 at 90 deg azimuth): X sin(A+90) + Y cos(A+90) -> Y cosA - X sinA
        // (Assuming horizontal side mic)
        float side = y * ca - x * sa;

        float l = mid + width * side;
        float r = mid - width * side;

        l *= level;
        r *= level;

        outL[i] += l;
        outR[i] += r;
    }
}