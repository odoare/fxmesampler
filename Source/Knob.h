/*
  ==============================================================================

    Knob.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class Knob : public juce::Slider
{
public:
    Knob();
    ~Knob() override;

    void mouseDown(const juce::MouseEvent& e) override;
};