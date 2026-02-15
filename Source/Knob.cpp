/*
  ==============================================================================

    Knob.cpp

  ==============================================================================
*/

#include "Knob.h"

Knob::Knob()
{
    setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
}

Knob::~Knob() {}

void Knob::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        auto* label = new juce::Label();
        label->setEditable(true);
        label->setText(getTextFromValue(getValue()), juce::NotificationType::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        
        label->setColour(juce::Label::backgroundColourId, juce::Colours::black);
        label->setColour(juce::Label::textColourId, juce::Colours::white);
        label->setColour(juce::Label::outlineColourId, juce::Colours::grey);
        
        auto bounds = getScreenBounds();
        label->setBounds(0, 0, juce::jmin(80, bounds.getWidth()), 20);
        label->setCentrePosition(bounds.getCentre());
        
        juce::Component::SafePointer<Knob> safeThis(this);
        
        label->onTextChange = [safeThis, label]() {
            if (safeThis)
                safeThis->setValue(safeThis->getValueFromText(label->getText()));
        };
        
        label->onEditorHide = [label]() {
            juce::MessageManager::getInstance()->callAsync([label]() { delete label; });
        };
        
        label->addToDesktop(juce::ComponentPeer::windowIsTemporary | juce::ComponentPeer::windowHasDropShadow);
        label->setVisible(true);
        label->showEditor();
    }
    else
    {
        juce::Slider::mouseDown(e);
    }
}