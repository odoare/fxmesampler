/*
  ==============================================================================

    EqualizerComponent.cpp

  ==============================================================================
*/

#include "EqualizerComponent.h"

//==============================================================================
// FrequencyResponseGraph Implementation
//==============================================================================

void FrequencyResponseGraph::setReferences (juce::Slider& lsF, juce::Slider& lsG,
                                            juce::Slider& b1F, juce::Slider& b1Qq, juce::Slider& b1G,
                                            juce::Slider& b2F, juce::Slider& b2Qq, juce::Slider& b2G,
                                            juce::Slider& hsF, juce::Slider& hsG,
                                            juce::Slider& postG,
                                            juce::ToggleButton& onB)
{
    lsFreq = &lsF; lsGain = &lsG;
    b1Freq = &b1F; b1Q = &b1Qq; b1Gain = &b1G;
    b2Freq = &b2F; b2Q = &b2Qq; b2Gain = &b2G;
    hsFreq = &hsF; hsGain = &hsG;
    postGain = &postG;
    onBtn = &onB;
    updateCurve();
}

static double getMagnitude (double b0, double b1, double b2, double a1, double a2, double freq, double sr)
{
    double w = 2.0 * juce::MathConstants<double>::pi * freq / sr;
    std::complex<double> z (std::cos(w), std::sin(w)); // z = e^(jw)
    // H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2)
    // We use z^-1 = 1/z = conj(z) since |z|=1
    std::complex<double> z_1 = std::conj(z);
    std::complex<double> z_2 = z_1 * z_1;
    std::complex<double> num = b0 + b1 * z_1 + b2 * z_2;
    std::complex<double> den = 1.0 + a1 * z_1 + a2 * z_2;
    return std::abs(num) / std::abs(den);
}

void FrequencyResponseGraph::updateCurve()
{
    if (lsFreq == nullptr) return;

    curvePath.clear();
    
    double sr = 44100.0; // Assume 44.1k for visualization
    int w = getWidth();
    if (w <= 0) w = 1;

    auto calcCoeffs = [&](double f, double q, double g, int type, double& b0, double& b1, double& b2, double& a1, double& a2)
    {
        double A = std::pow (10.0, g / 40.0);
        double w0 = 2.0 * juce::MathConstants<double>::pi * f / sr;
        double cos_w0 = std::cos (w0);
        double sin_w0 = std::sin (w0);
        double alpha = 0.0;

        if (type == 0 || type == 2) // Shelf
            alpha = sin_w0 / 2.0 * std::sqrt ((A + 1.0 / A) * (1.0 / 1.0 - 1.0) + 2.0);
        else // Peak
            alpha = sin_w0 / (2.0 * q);

        if (type == 0) // Low Shelf
        {
            double a0 = (A + 1.0) + (A - 1.0) * cos_w0 + 2.0 * std::sqrt (A) * alpha;
            b0 = A * ((A + 1.0) - (A - 1.0) * cos_w0 + 2.0 * std::sqrt (A) * alpha) / a0;
            b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos_w0) / a0;
            b2 = A * ((A + 1.0) - (A - 1.0) * cos_w0 - 2.0 * std::sqrt (A) * alpha) / a0;
            a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos_w0) / a0;
            a2 = ((A + 1.0) + (A - 1.0) * cos_w0 - 2.0 * std::sqrt (A) * alpha) / a0;
        }
        else if (type == 1) // Peak
        {
            double a0 = 1.0 + alpha / A;
            b0 = (1.0 + alpha * A) / a0;
            b1 = (-2.0 * cos_w0) / a0;
            b2 = (1.0 - alpha * A) / a0;
            a1 = (-2.0 * cos_w0) / a0;
            a2 = (1.0 - alpha / A) / a0;
        }
        else // High Shelf
        {
            double a0 = (A + 1.0) - (A - 1.0) * cos_w0 + 2.0 * std::sqrt (A) * alpha;
            b0 = A * ((A + 1.0) + (A - 1.0) * cos_w0 + 2.0 * std::sqrt (A) * alpha) / a0;
            b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos_w0) / a0;
            b2 = A * ((A + 1.0) + (A - 1.0) * cos_w0 - 2.0 * std::sqrt (A) * alpha) / a0;
            a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cos_w0) / a0;
            a2 = ((A + 1.0) - (A - 1.0) * cos_w0 - 2.0 * std::sqrt (A) * alpha) / a0;
        }
    };

    double b0_ls, b1_ls, b2_ls, a1_ls, a2_ls;
    calcCoeffs (lsFreq->getValue(), 0.0, lsGain->getValue(), 0, b0_ls, b1_ls, b2_ls, a1_ls, a2_ls);

    double b0_b1, b1_b1, b2_b1, a1_b1, a2_b1;
    calcCoeffs (b1Freq->getValue(), b1Q->getValue(), b1Gain->getValue(), 1, b0_b1, b1_b1, b2_b1, a1_b1, a2_b1);

    double b0_b2, b1_b2, b2_b2, a1_b2, a2_b2;
    calcCoeffs (b2Freq->getValue(), b2Q->getValue(), b2Gain->getValue(), 1, b0_b2, b1_b2, b2_b2, a1_b2, a2_b2);

    double b0_hs, b1_hs, b2_hs, a1_hs, a2_hs;
    calcCoeffs (hsFreq->getValue(), 0.0, hsGain->getValue(), 2, b0_hs, b1_hs, b2_hs, a1_hs, a2_hs);

    double globalGain = juce::Decibels::decibelsToGain (postGain->getValue());

    for (int x = 0; x < w; ++x)
    {
        double freq = 20.0 * std::pow (20000.0 / 20.0, x / (double) w);
        double mag = globalGain;
        mag *= getMagnitude (b0_ls, b1_ls, b2_ls, a1_ls, a2_ls, freq, sr);
        mag *= getMagnitude (b0_b1, b1_b1, b2_b1, a1_b1, a2_b1, freq, sr);
        mag *= getMagnitude (b0_b2, b1_b2, b2_b2, a1_b2, a2_b2, freq, sr);
        mag *= getMagnitude (b0_hs, b1_hs, b2_hs, a1_hs, a2_hs, freq, sr);

        double db = juce::Decibels::gainToDecibels (mag);
        double y = juce::jmap (db, -24.0, 24.0, (double) getHeight(), 0.0);

        if (x == 0) curvePath.startNewSubPath ((float) x, (float) y);
        else        curvePath.lineTo ((float) x, (float) y);
    }
    repaint();
}

void FrequencyResponseGraph::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.3f));

    g.setColour (juce::Colours::grey);
    for (float freq : { 100.0f, 1000.0f, 10000.0f })
    {
        float x = (float) getWidth() * (std::log (freq / 20.0f) / std::log (20000.0f / 20.0f));
        g.drawVerticalLine ((int) x, 0.0f, (float) getHeight());
    }

    g.setColour (juce::Colours::darkgrey.darker());
    for (float freq : { 20.0f, 30.f, 40.f, 50.f, 60.f, 70.f, 80.f, 90.f,
                        200.0f, 300.f, 400.f, 500.f, 600.f, 700.f, 800.f, 900.f,
                        2000.0f, 3000.f, 4000.f, 5000.f, 6000.f, 7000.f, 8000.f, 9000.f, })
    {
        float x = (float) getWidth() * (std::log (freq / 20.0f) / std::log (20000.0f / 20.0f));
        g.drawVerticalLine ((int) x, 0.0f, (float) getHeight());
    }

    g.setColour (juce::Colours::darkgrey.darker());
    for (float amp : { -12.f, 0.f, 12.f })
    {
        float y = juce::jmap (amp, -24.0f, 24.0f, (float) getHeight(), 0.0f);
        g.drawHorizontalLine ((int) y, 0.0f, (float) getWidth());
    }

    if (onBtn && onBtn->getToggleState())
        g.setColour (juce::Colours::cyan);
    else
        g.setColour (juce::Colours::grey);
    g.strokePath (curvePath, juce::PathStrokeType (2.0f));
    g.drawRect (getLocalBounds(), 1);
}

//==============================================================================
// EqualizerComponent Implementation
//==============================================================================

void EqualizerComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

EqualizerComponent::EqualizerComponent (Equalizer& eq, juce::AudioProcessorValueTreeState& state, const juce::String& prefix)
    : equalizer (eq), apvts (state)
{
    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onButton.setLookAndFeel(&fxmeLookAndFeel);
    onButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::cyan);
    onAtt = std::make_unique<ButtonAttachment> (apvts, prefix + "_EQ_On", onButton);
    onButton.onClick = [this] { responseGraph.updateCurve(); };

    addAndMakeVisible (titleLabel);
    titleLabel.setText ("Equalizer (LS / Peak / Peak / HS)", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (16.0f, juce::Font::bold));

    juce::Colour color = juce::Colours::cyan;

    auto setup = [&](juce::Slider& s, double min, double max, double def, const juce::String& tooltip) {
        addAndMakeVisible (s);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRange (min, max);
        s.setValue (def);
        s.setTooltip(tooltip);
        s.setLookAndFeel(&fxmeLookAndFeel);
        setSliderColours(s, color);
        s.onValueChange = [this] { responseGraph.updateCurve(); };
    };

    // Low Shelf
    setup (lsFreq, 20.0, 1000.0, 100.0, "LS Freq");
    setup (lsGain, -15.0, 15.0, 0.0, "LS Gain");
    lsFreqAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_LS_Freq", lsFreq);
    lsGainAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_LS_Gain", lsGain);

    // Band 1
    setup (b1Freq, 100.0, 5000.0, 500.0, "Peak 1 Freq");
    setup (b1Q, 0.1, 10.0, 1.0, "Peak 1 Q");
    setup (b1Gain, -15.0, 15.0, 0.0, "Peak 1 Gain");
    b1FreqAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_B1_Freq", b1Freq);
    b1QAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_B1_Q", b1Q);
    b1GainAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_B1_Gain", b1Gain);

    // Band 2
    setup (b2Freq, 500.0, 10000.0, 2000.0, "Peak 2 Freq");
    setup (b2Q, 0.1, 10.0, 1.0, "Peak 2 Q");
    setup (b2Gain, -15.0, 15.0, 0.0, "Peak 2 Gain");
    b2FreqAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_B2_Freq", b2Freq);
    b2QAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_B2_Q", b2Q);
    b2GainAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_B2_Gain", b2Gain);

    // High Shelf
    setup (hsFreq, 1000.0, 20000.0, 5000.0, "HS Freq");
    setup (hsGain, -15.0, 15.0, 0.0, "HS Gain");
    hsFreqAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_HS_Freq", hsFreq);
    hsGainAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_HS_Gain", hsGain);

    addAndMakeVisible (postGainSlider);
    postGainSlider.setSliderStyle (juce::Slider::LinearBarVertical);
    postGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    postGainSlider.setRange (-24.0, 24.0, 0.1);
    postGainSlider.setValue (0.0);
    postGainSlider.setTextValueSuffix ("dB");
    postGainSlider.setTooltip ("Post Gain");
    postGainSlider.setLookAndFeel(&fxmeLookAndFeel);
    setSliderColours(postGainSlider, color);
    postGainSlider.onValueChange = [this] { responseGraph.updateCurve(); };
    postGainAtt = std::make_unique<SliderAttachment> (apvts, prefix + "_EQ_PostGain", postGainSlider);

    addAndMakeVisible (responseGraph);
    responseGraph.setReferences (lsFreq, lsGain,
                                 b1Freq, b1Q, b1Gain,
                                 b2Freq, b2Q, b2Gain,
                                 hsFreq, hsGain, postGainSlider, onButton);
}

EqualizerComponent::~EqualizerComponent()
{
}

void EqualizerComponent::paint (juce::Graphics& g)
{
    auto diagonale = (getLocalBounds().getTopLeft() - getLocalBounds().getBottomRight()).toFloat();
    auto length = diagonale.getDistanceFromOrigin();
    auto perpendicular = diagonale.rotatedAboutOrigin (juce::degreesToRadians (270.0f)) / length;
    auto height = float (getWidth() * getHeight()) / length;
    //auto bluegreengrey = juce::Colour::fromFloatRGBA (0.15f, 0.15f, 0.25f, 1.0f);
    auto bluegreengrey = juce::Colours::cyan.darker(3.f);
    juce::ColourGradient grad (bluegreengrey.darker().darker().darker(), perpendicular * height,
                           bluegreengrey, perpendicular * -height, false);
    g.setGradientFill(grad);
    g.fillAll();
}

void EqualizerComponent::resized()
{
    auto area = getLocalBounds().reduced (5);
    using fi = juce::FlexItem;
    juce::FlexBox fmain, ftop, fbottom,
                    feq1, feq2, feq3, feq4, feq;
    
    ftop.flexDirection = juce::FlexBox::Direction::row;
    fbottom.flexDirection = juce::FlexBox::Direction::row;
    feq1.flexDirection = juce::FlexBox::Direction::column;
    feq2.flexDirection = juce::FlexBox::Direction::column;
    feq3.flexDirection = juce::FlexBox::Direction::column;
    feq4.flexDirection = juce::FlexBox::Direction::column;
    feq.flexDirection = juce::FlexBox::Direction::row;
    fmain.flexDirection = juce::FlexBox::Direction::column;

    ftop.items.add(fi(onButton).withFlex(0.2f));
    ftop.items.add(fi(titleLabel).withFlex(1.5f));
    
    feq1.items.add(fi(lsFreq).withFlex(1.f));
    feq1.items.add(fi(lsGain).withFlex(1.f));
    feq2.items.add(fi(b1Freq).withFlex(1.f));
    feq2.items.add(fi(b1Q).withFlex(1.f));
    feq2.items.add(fi(b1Gain).withFlex(1.f));
    feq3.items.add(fi(b2Freq).withFlex(1.f));
    feq3.items.add(fi(b2Q).withFlex(1.f));
    feq3.items.add(fi(b2Gain).withFlex(1.f));
    feq4.items.add(fi(hsFreq).withFlex(1.f));
    feq4.items.add(fi(hsGain).withFlex(1.f));
    feq.items.add(fi(feq1).withFlex(1.f));
    feq.items.add(fi(feq2).withFlex(1.f));
    feq.items.add(fi(feq3).withFlex(1.f));
    feq.items.add(fi(feq4).withFlex(1.f));
    fbottom.items.add(fi(postGainSlider).withFlex(0.2f).withMargin(juce::FlexItem::Margin(0.f, 20.f, 0.f, 10.f)));
    fbottom.items.add(fi(responseGraph).withFlex(1.4f).withMargin(juce::FlexItem::Margin(0.f, 10.f, 0.f, 0)));
    fmain.items.add(fi(ftop).withFlex(0.13f).withMargin(juce::FlexItem::Margin(5.f, 0.f, 10.f, 0)));
    fmain.items.add(fi(feq).withFlex(1.f));
    fmain.items.add(fi(fbottom).withFlex(0.85f).withMargin(juce::FlexItem::Margin(10.f, 0.f, 0.f, 0)));

    fmain.performLayout(area);

}