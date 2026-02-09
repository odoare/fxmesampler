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
    irPlotPathL.clear();
    irPlotPathR.clear();

    const auto& buffer = reverb.getModifiedIR();
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    if (numSamples == 0) return;
    
    float w = (float)getWidth();
    // Graph area starts at 40% of height (below sliders)
    float graphTop = (float)getHeight() * 0.4f;
    float graphHeight = (float)getHeight() * 0.6f;

    if (w <= 0 || graphHeight <= 0) return;

    auto drawChannel = [&](int channel, float centerY, float height)
    {
        juce::Path& p = (channel == 0) ? irPlotPathL : irPlotPathR;
        auto* data = buffer.getReadPointer(channel);

        p.startNewSubPath (0, centerY);

        int resolution = juce::jmin(numSamples, (int)w);
        int step = numSamples / resolution;
        if (step == 0) step = 1;

        float maxVal = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            maxVal = juce::jmax(maxVal, std::abs(data[i]));
        if (maxVal == 0.0f) maxVal = 1.0f;

        for (int i = 0; i < resolution; ++i)
        {
            int start = i * step;
            int end = juce::jmin (start + step, numSamples);
            
            float sumSq = 0.0f;
            for (int k = start; k < end; ++k)
                sumSq += data[k] * data[k];
            
            float val = (end > start) ? std::sqrt (sumSq / (float) (end - start)) : 0.0f;
            float x = (float)i / (float)resolution * w;
            float y = centerY - (val / maxVal * height * 0.5f);
            
            if (i == 0)
                p.startNewSubPath (x, y);
            else
                p.lineTo (x, y);
        }
    };

    if (numChannels == 1)
    {
        drawChannel(0, graphTop + graphHeight * 0.5f, graphHeight * 0.9f);
    }
    else
    {
        drawChannel(0, graphTop + graphHeight * 0.25f, graphHeight * 0.45f);
        drawChannel(1, graphTop + graphHeight * 0.75f, graphHeight * 0.45f);
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
    g.strokePath (irPlotPathL, juce::PathStrokeType (1.5f));
    if (!irPlotPathR.isEmpty())
        g.strokePath (irPlotPathR, juce::PathStrokeType (1.5f));
    
    // Draw a horizontal line for the zero crossing
    g.setColour (juce::Colours::white.withAlpha(0.3f));
    
    float graphTop = (float)getHeight() * 0.4f;
    float graphHeight = (float)getHeight() * 0.6f;

    if (irPlotPathR.isEmpty())
    {
        float y = graphTop + graphHeight * 0.5f;
        g.drawLine (0, y, (float)getWidth(), y, 1.0f);
    }
    else
    {
        float y1 = graphTop + graphHeight * 0.25f;
        float y2 = graphTop + graphHeight * 0.75f;
        g.drawLine (0, y1, (float)getWidth(), y1, 1.0f);
        g.drawLine (0, y2, (float)getWidth(), y2, 1.0f);
    }
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