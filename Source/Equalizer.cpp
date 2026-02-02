/*
  ==============================================================================

    Equalizer.cpp

  ==============================================================================
*/

#include "Equalizer.h"

Equalizer::Equalizer()
{
}

void Equalizer::prepare (double sampleRate, int numChannels)
{
    currentSampleRate = sampleRate;
    channels.resize (numChannels);
    for (auto& ch : channels)
    {
        ch.lowShelf.reset();
        ch.band1.reset();
        ch.band2.reset();
        ch.highShelf.reset();
    }
    updateCoefficients();
}

void Equalizer::setLowShelf (float freq, float gaindB)
{
    lsParams.f = freq; lsParams.g = gaindB;
    updateCoefficients();
}

void Equalizer::setBand1 (float freq, float Q, float gaindB)
{
    b1Params.f = freq; b1Params.q = Q; b1Params.g = gaindB;
    updateCoefficients();
}

void Equalizer::setBand2 (float freq, float Q, float gaindB)
{
    b2Params.f = freq; b2Params.q = Q; b2Params.g = gaindB;
    updateCoefficients();
}

void Equalizer::setHighShelf (float freq, float gaindB)
{
    hsParams.f = freq; hsParams.g = gaindB;
    updateCoefficients();
}

void Equalizer::setOn (bool shouldBeOn)
{
    on = shouldBeOn;
}

bool Equalizer::isOn() const
{
    return on;
}

void Equalizer::updateCoefficients()
{
    for (auto& ch : channels)
    {
        calcLowShelf (ch.lowShelf, lsParams.f, lsParams.g);
        calcPeaking (ch.band1, b1Params.f, b1Params.q, b1Params.g);
        calcPeaking (ch.band2, b2Params.f, b2Params.q, b2Params.g);
        calcHighShelf (ch.highShelf, hsParams.f, hsParams.g);
    }
}

float Equalizer::Biquad::process (float in)
{
    float out = b0 * in + z1;
    z1 = b1 * in + z2 - a1 * out;
    z2 = b2 * in - a2 * out;
    return out;
}

void Equalizer::process (juce::AudioBuffer<float>& buffer)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    // Ensure we have enough channel strips
    if (numChannels > (int) channels.size())
        prepare (currentSampleRate, numChannels);

    if (! on)
        return;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        auto& strip = channels[ch];

        for (int i = 0; i < numSamples; ++i)
        {
            float s = data[i];
            s = strip.lowShelf.process (s);
            s = strip.band1.process (s);
            s = strip.band2.process (s);
            s = strip.highShelf.process (s);
            data[i] = s;
        }
    }
}

// RBJ Cookbook Formulas
void Equalizer::calcLowShelf (Biquad& bq, float f, float g)
{
    float A = std::pow (10.0f, g / 40.0f);
    float w0 = 2.0f * juce::MathConstants<float>::pi * f / (float) currentSampleRate;
    float cos_w0 = std::cos (w0);
    float sin_w0 = std::sin (w0);
    float alpha = sin_w0 / 2.0f * std::sqrt ((A + 1.0f / A) * (1.0f / 1.0f - 1.0f) + 2.0f); // S = 1

    float b0 =    A * ((A + 1.0f) - (A - 1.0f) * cos_w0 + 2.0f * std::sqrt (A) * alpha);
    float b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cos_w0);
    float b2 =    A * ((A + 1.0f) - (A - 1.0f) * cos_w0 - 2.0f * std::sqrt (A) * alpha);
    float a0 =        (A + 1.0f) + (A - 1.0f) * cos_w0 + 2.0f * std::sqrt (A) * alpha;
    float a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cos_w0);
    float a2 =        (A + 1.0f) + (A - 1.0f) * cos_w0 - 2.0f * std::sqrt (A) * alpha;

    bq.b0 = b0 / a0;
    bq.b1 = b1 / a0;
    bq.b2 = b2 / a0;
    bq.a1 = a1 / a0;
    bq.a2 = a2 / a0;
}

void Equalizer::calcPeaking (Biquad& bq, float f, float q, float g)
{
    float A = std::pow (10.0f, g / 40.0f);
    float w0 = 2.0f * juce::MathConstants<float>::pi * f / (float) currentSampleRate;
    float cos_w0 = std::cos (w0);
    float sin_w0 = std::sin (w0);
    float alpha = sin_w0 / (2.0f * q);

    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * cos_w0;
    float b2 = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * cos_w0;
    float a2 = 1.0f - alpha / A;

    bq.b0 = b0 / a0;
    bq.b1 = b1 / a0;
    bq.b2 = b2 / a0;
    bq.a1 = a1 / a0;
    bq.a2 = a2 / a0;
}

void Equalizer::calcHighShelf (Biquad& bq, float f, float g)
{
    float A = std::pow (10.0f, g / 40.0f);
    float w0 = 2.0f * juce::MathConstants<float>::pi * f / (float) currentSampleRate;
    float cos_w0 = std::cos (w0);
    float sin_w0 = std::sin (w0);
    float alpha = sin_w0 / 2.0f * std::sqrt ((A + 1.0f / A) * (1.0f / 1.0f - 1.0f) + 2.0f); // S = 1

    float b0 =    A * ((A + 1.0f) + (A - 1.0f) * cos_w0 + 2.0f * std::sqrt (A) * alpha);
    float b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cos_w0);
    float b2 =    A * ((A + 1.0f) + (A - 1.0f) * cos_w0 - 2.0f * std::sqrt (A) * alpha);
    float a0 =        (A + 1.0f) - (A - 1.0f) * cos_w0 + 2.0f * std::sqrt (A) * alpha;
    float a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cos_w0);
    float a2 =        (A + 1.0f) - (A - 1.0f) * cos_w0 - 2.0f * std::sqrt (A) * alpha;

    bq.b0 = b0 / a0;
    bq.b1 = b1 / a0;
    bq.b2 = b2 / a0;
    bq.a1 = a1 / a0;
    bq.a2 = a2 / a0;
}