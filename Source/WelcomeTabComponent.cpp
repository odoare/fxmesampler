/*
  ==============================================================================

    WelcomeTabComponent.cpp

  ==============================================================================
*/

#include "WelcomeTabComponent.h"

WelcomeTabComponent::WelcomeTabComponent(juce::Component& content, juce::AudioProcessorValueTreeState& apvts)
    : content(content), apvts(apvts)
{
    addAndMakeVisible(content);
    
    addAndMakeVisible(saveButton);
    saveButton.setButtonText("Save Preset");
    saveButton.onClick = [this] { savePreset(); };
    
    addAndMakeVisible(loadButton);
    loadButton.setButtonText("Load Preset");
    loadButton.onClick = [this] { loadPreset(); };
}

WelcomeTabComponent::~WelcomeTabComponent()
{
}

void WelcomeTabComponent::resized()
{
    auto area = getLocalBounds();
    auto buttonArea = area.removeFromBottom(40).reduced(5);
    
    saveButton.setBounds(buttonArea.removeFromLeft(100));
    buttonArea.removeFromLeft(10);
    loadButton.setBounds(buttonArea.removeFromLeft(100));
    
    content.setBounds(area);
}

void WelcomeTabComponent::savePreset()
{
    auto file = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("preset.xml");
    fc = std::make_unique<juce::FileChooser>("Save Preset", file, "*.xml");
    fc->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser) {
            auto result = chooser.getResult();
            if (result != juce::File()) {
                auto state = apvts.copyState();
                std::unique_ptr<juce::XmlElement> xml(state.createXml());
                if (xml) xml->writeTo(result);
            }
        });
}

void WelcomeTabComponent::loadPreset()
{
    fc = std::make_unique<juce::FileChooser>("Load Preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.xml");
    fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser) {
            auto result = chooser.getResult();
            if (result != juce::File()) {
                auto xml = juce::XmlDocument::parse(result);
                if (xml) apvts.replaceState(juce::ValueTree::fromXml(*xml));
            }
        });
}