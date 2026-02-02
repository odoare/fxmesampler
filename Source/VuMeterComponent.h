/*
  ==============================================================================

    VuMeterComponent.h
    Created: 2 Feb 2026 9:21:06am
    Author:  doare

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "VuMeter.h"

class VuMeterComponent : public juce::Component,
                          public juce::Timer
{
public:
    VuMeterComponent();
    ~VuMeterComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setValue (float newValue);
    void setMeterColor (juce::Colour newColor);
    void setRange (float newMin, float newMax);
    void setZeroLevel (float newZeroLevel);

private:
    float value = -100.0f;
    juce::Colour meterColor = juce::Colours::green;
    float minValue = -60.0f;
    float maxValue = 0.0f;
    float zeroLevel = -100.0f; // -Inf
    
    void timerCallback() override;
};
