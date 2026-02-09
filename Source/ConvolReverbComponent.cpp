/*
  ==============================================================================

    ConvolReverbComponent.cpp

  ==============================================================================
*/

#include "ConvolReverbComponent.h"

void ConvolReverbComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

void ConvolReverbComponent::setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, double min, double max, double def)
{
    juce::Colour color = juce::Colours::lightgreen;

    addAndMakeVisible (label);
    label.setText (text, juce::NotificationType::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange (min, max);
    slider.setValue (def);
    slider.setTooltip (text);
    slider.setLookAndFeel(&fxmeLookAndFeel);
    setSliderColours(slider, color);
}

ConvolReverbComponent::ConvolReverbComponent (ConvolReverb& r, juce::AudioProcessorValueTreeState& state, const juce::String& prefix)
    : reverb (r), apvts (state)
{
    addAndMakeVisible (titleLabel);
    titleLabel.setText ("Convolution Reverb", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (16.0f, juce::Font::bold));

    addAndMakeVisible (irLabel);
    irLabel.setText ("Impulse", juce::NotificationType::dontSendNotification);
    irLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (irBox);
    const auto& names = reverb.getImpulseNames();
    for (int i = 0; i < names.size(); ++i)
        irBox.addItem (names[i], i + 1);
    irAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, prefix + "_IR", irBox);
    irBox.onChange = [this] { updateGraph(); };

    setupSlider (lengthSlider, lengthLabel, "Length", 0.0, 1.0, 1.0);
    lengthAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, prefix + "_Length", lengthSlider);
    lengthSlider.onValueChange = [this] { updateGraph(); };

    addAndMakeVisible (shapeLabel);
    shapeLabel.setText ("Shape", juce::NotificationType::dontSendNotification);
    shapeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (shapeBox);
    shapeBox.addItem ("Fast Exp", 1);
    shapeBox.addItem ("Linear", 2);
    shapeBox.addItem ("Slow Log", 3);
    shapeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, prefix + "_Shape", shapeBox);
    shapeBox.onChange = [this] { updateGraph(); };
    
    startTimerHz (24); // Update graph periodically
    updateGraph();
}

ConvolReverbComponent::~ConvolReverbComponent() {}

void ConvolReverbComponent::updateGraph()
{
    irPlotPath.clear();
    const auto& buffer = reverb.getModifiedIR();
    if (buffer.getNumSamples() == 0) return;

    auto* data = buffer.getReadPointer(0); // Use left channel for viz
    int numSamples = buffer.getNumSamples();
    
    float w = (float)getWidth();
    float h = (float)getHeight() * 0.4f; // Graph takes 40% height
    float yOffset = (float)getHeight() * 0.5f; // Center the graph vertically

    if (w <= 0 || h <= 0) return;

    irPlotPath.startNewSubPath (0, yOffset);

    int resolution = juce::jmin(numSamples, (int)w); // Max resolution is width of component
    int step = numSamples / resolution;
    if (step == 0) step = 1;

    float maxVal = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        maxVal = juce::jmax(maxVal, std::abs(data[i]));
    if (maxVal == 0.0f) maxVal = 1.0f; // Avoid division by zero

    for (int i = 0; i < resolution; ++i)
    {
        int sampleIdx = i * step;
        if (sampleIdx >= numSamples) sampleIdx = numSamples - 1;
        
        float val = data[sampleIdx];
        float x = (float)i / (float)resolution * w;
        float y = yOffset - (val / maxVal * h * 0.5f); // Scale to half height and center
        
        if (i == 0)
            irPlotPath.startNewSubPath (x, y);
        else
            irPlotPath.lineTo (x, y);
    }
    repaint();
}

void ConvolReverbComponent::timerCallback()
{
    // The graph updates on parameter changes, but a timer can ensure it's always up-to-date
    // if there are external changes or for continuous animation (not implemented here).
    // For now, it's mainly for repainting.
    // updateGraph(); // Only call if there are actual changes, otherwise it's too heavy
    repaint();
}

void ConvolReverbComponent::paint (juce::Graphics& g)
{
    auto diagonale = (getLocalBounds().getTopLeft() - getLocalBounds().getBottomRight()).toFloat();
    auto length = diagonale.getDistanceFromOrigin();
    auto perpendicular = diagonale.rotatedAboutOrigin (juce::degreesToRadians (270.0f)) / length;
    auto height = float (getWidth() * getHeight()) / length;
    auto bluegreengrey = juce::Colour::fromFloatRGBA (0.15f, 0.15f, 0.25f, 1.0f);
    juce::ColourGradient grad (bluegreengrey.darker().darker().darker(), perpendicular * height,
                           bluegreengrey, perpendicular * -height, false);
    g.setGradientFill(grad);
    g.fillAll();

    g.setColour (juce::Colours::lightgreen.withAlpha(0.7f));
    g.strokePath (irPlotPath, juce::PathStrokeType (1.5f));
    
    // Draw a horizontal line for the zero crossing
    g.setColour (juce::Colours::white.withAlpha(0.3f));
    g.drawLine (0, getHeight() * 0.5f, getWidth(), getHeight() * 0.5f, 1.0f);
}

void ConvolReverbComponent::resized()
{
    auto area = getLocalBounds().reduced (5);
    using fi = juce::FlexItem;
    juce::FlexBox fMain, fTop, fSliders;
    fMain.flexDirection = juce::FlexBox::Direction::column;
    fTop.flexDirection = juce::FlexBox::Direction::row;
    fSliders.flexDirection = juce::FlexBox::Direction::row;

    fTop.items.add(fi(titleLabel).withFlex(1.f));

    fSliders.items.add(fi(irLabel).withFlex(0.2f));
    fSliders.items.add(fi(irBox).withFlex(0.8f));
    fSliders.items.add(fi(lengthLabel).withFlex(0.2f));
    fSliders.items.add(fi(lengthSlider).withFlex(0.8f));
    fSliders.items.add(fi(shapeLabel).withFlex(0.2f));
    fSliders.items.add(fi(shapeBox).withFlex(0.8f));

    fMain.items.add(fi(fTop).withFlex(0.1f));
    fMain.items.add(fi(fSliders).withFlex(0.3f));
    // Graph will take the remaining space
    fMain.items.add(fi().withFlex(0.6f)); // Placeholder for graph area

    fMain.performLayout(area);

    // Manually set bounds for the graph area
    juce::Rectangle<int> graphArea = area;
    graphArea.removeFromTop(area.getHeight() * 0.4f); // Sliders and title take 40%
    // The graph is drawn in paint, so we just need to make sure its bounds are correct for drawing.
    // The flexbox layout above already reserves space.

    updateGraph(); // Update graph after resize
}