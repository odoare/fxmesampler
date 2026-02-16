/*
  ==============================================================================

    FxmeSlider.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class FxmeSlider : public juce::Slider
{
public:
    FxmeSlider();
    ~FxmeSlider() override;

    void mouseDown(const juce::MouseEvent& e) override;
};