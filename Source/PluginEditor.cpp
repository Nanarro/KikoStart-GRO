#include "PluginEditor.h"

using namespace juce;

namespace kg::ui
{
static Font boldFont (float size, int extraStyle = 0)
{
    return Font (FontOptions (size, Font::bold | extraStyle));
}

//==============================================================================
// LookAndFeel
//==============================================================================
void LookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                    float startAngle, float endAngle, Slider&)
{
    auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float arcW = radius * 0.11f;
    const float arcR = radius - arcW * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (Colour (0xff3b3b3b));
    g.strokePath (track, PathStrokeType (arcW, PathStrokeType::curved, PathStrokeType::rounded));

    Path value;
    value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
    g.setColour (yellow);
    g.strokePath (value, PathStrokeType (arcW, PathStrokeType::curved, PathStrokeType::rounded));

    const float innerR = radius - arcW - radius * 0.08f;
    g.setColour (Colour (0xff2f2f2f));
    g.fillEllipse (centre.x - innerR, centre.y - innerR, innerR * 2.0f, innerR * 2.0f);
    g.setColour (Colour (0xff404040));
    g.drawEllipse (centre.x - innerR, centre.y - innerR, innerR * 2.0f, innerR * 2.0f, 2.0f);

    const float holeR = innerR * 0.42f;
    g.setColour (Colour (0xff161616));
    g.fillEllipse (centre.x - holeR, centre.y - holeR, holeR * 2.0f, holeR * 2.0f);
    g.setColour (Colour (0xff4a4a4a));
    g.drawEllipse (centre.x - holeR, centre.y - holeR, holeR * 2.0f, holeR * 2.0f, 1.5f);

    const auto dot = centre.getPointOnCircumference (innerR * 0.76f, angle);
    const float dr = radius * 0.04f;
    g.setColour (Colour (0xff9a9a9a));
    g.fillEllipse (dot.x - dr, dot.y - dr, dr * 2.0f, dr * 2.0f);
}

void LookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float sliderPos,
                                    float, float, Slider::SliderStyle, Slider&)
{
    const float cy = (float) y + (float) h * 0.5f;
    const float th = 8.0f;
    Rectangle<float> track ((float) x + 9.0f, cy - th * 0.5f, (float) w - 18.0f, th);

    g.setColour (Colour (0xff3b3b3b));
    g.fillRoundedRectangle (track, th * 0.5f);

    g.setColour (yellow);
    g.fillRoundedRectangle (track.withRight (jmax (track.getX() + th, sliderPos)), th * 0.5f);

    g.setColour (Colour (0xffc4c4c4));
    g.fillEllipse (sliderPos - 9.0f, cy - 9.0f, 18.0f, 18.0f);
    g.setColour (Colour (0xff2a2a2a));
    g.fillEllipse (sliderPos - 4.0f, cy - 4.0f, 8.0f, 8.0f);
}

void LookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    g.setColour (on ? yellow : (highlighted ? Colour (0xff3a3a3a) : panel));
    g.fillRoundedRectangle (r, jmin (r.getHeight() * 0.5f, 12.0f));
}

void LookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    g.setColour (b.getToggleState() ? Colours::black : Colour (0xffd2d2d2));
    g.setFont (boldFont (13.0f));
    g.drawText (b.getButtonText(), b.getLocalBounds(), Justification::centred);
}

//==============================================================================
// HeadphoneButton
//==============================================================================
void HeadphoneButton::paintButton (Graphics& g, bool over, bool)
{
    const auto r = getLocalBounds().toFloat();
    g.setColour (getToggleState() ? yellow : (over ? Colours::white : Colour (0xffbdbdbd)));

    const auto c = r.getCentre();
    const float rx = r.getWidth() * 0.30f;
    const float ry = r.getHeight() * 0.30f;

    Path arc;
    arc.addCentredArc (c.x, c.y + 1.0f, rx, ry, 0.0f,
                       -MathConstants<float>::halfPi, MathConstants<float>::halfPi, true);
    g.strokePath (arc, PathStrokeType (2.2f));
    g.fillRoundedRectangle (c.x - rx - 3.0f, c.y + 1.0f, 5.0f, 8.0f, 2.0f);
    g.fillRoundedRectangle (c.x + rx - 2.0f, c.y + 1.0f, 5.0f, 8.0f, 2.0f);
}

//==============================================================================
// CurveDisplay
//==============================================================================
CurveDisplay::CurveDisplay (KikoStartAudioProcessor& p) : proc (p)
{
    shiftParam  = proc.apvts.getParameter ("shift");
    lengthParam = proc.apvts.getParameter ("length");
    shiftV  = proc.apvts.getRawParameterValue ("shift");
    lengthV = proc.apvts.getRawParameterValue ("length");
    curveV  = proc.apvts.getRawParameterValue ("curve");
    modeV   = proc.apvts.getRawParameterValue ("mode");
    setMouseCursor (MouseCursor::DraggingHandCursor);
}

float CurveDisplay::handleX() const
{
    return jlimit (0.0f, 1.0f, shiftV->load() + lengthV->load()) * (float) getWidth();
}

void CurveDisplay::setPlain (RangedAudioParameter* p, float v)
{
    p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void CurveDisplay::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float w = b.getWidth(), h = b.getHeight();

    Path clip;
    clip.addRoundedRectangle (b, 6.0f);
    g.reduceClipRegion (clip);

    g.setColour (screen);
    g.fillRect (b);

    // grid
    g.setColour (Colour (0xff202020));
    for (int i = 1; i < 8; ++i)
        g.fillRect (w * (float) i / 8.0f, 0.0f, 1.0f, h);
    g.setColour (Colour (0xff2e2e2e));
    g.fillRect (0.0f, h * 0.5f, w, 1.0f);
    for (int i = 1; i < 4; ++i)
        g.fillRect (w * (float) i / 4.0f, 0.0f, 1.0f, h);

    // input waveform
    constexpr int N = KikoStartAudioProcessor::numWaveBins;
    g.setColour (Colour (0xffb8b8b8).withAlpha (0.75f));
    for (int px = 0; px < (int) w; ++px)
    {
        const int b0 = jlimit (0, N - 1, (int) ((float) px / w * (float) N));
        const int b1 = jlimit (b0, N - 1, (int) ((float) (px + 1) / w * (float) N));
        float lo = 0.0f, hi = 0.0f;
        for (int k = b0; k <= b1; ++k)
        {
            lo = jmin (lo, proc.waveMin[(size_t) k].load (std::memory_order_relaxed));
            hi = jmax (hi, proc.waveMax[(size_t) k].load (std::memory_order_relaxed));
        }
        const float yTop = h * 0.5f - jlimit (-1.0f, 1.0f, hi) * h * 0.45f;
        float yBot = h * 0.5f - jlimit (-1.0f, 1.0f, lo) * h * 0.45f;
        if (yBot - yTop < 1.0f) yBot = yTop + 1.0f;
        g.fillRect ((float) px, yTop, 1.0f, yBot - yTop);
    }

    // area after the end of the curve
    const float hx = handleX();
    g.setColour (Colours::black.withAlpha (0.25f));
    g.fillRect (hx, 0.0f, w - hx, h);

    // volume curve
    const float shift = shiftV->load(), length = lengthV->load();
    const int curve = (int) curveV->load();
    const bool wrap = modeV->load() < 0.5f;
    const float pad = 16.0f;

    Path p;
    for (int px = 0; px <= (int) w; ++px)
    {
        const float t = (float) px / w;
        float u = t - shift;
        if (wrap) u -= std::floor (u);
        const float gain = kg::gainAt (curve, u, length);
        const float y = h - pad - gain * (h - 2.0f * pad);
        if (px == 0) p.startNewSubPath (0.0f, y); else p.lineTo ((float) px, y);
    }
    g.setColour (yellow);
    g.strokePath (p, PathStrokeType (2.8f, PathStrokeType::curved, PathStrokeType::rounded));

    // play-head
    const float t = proc.playPhase.load (std::memory_order_relaxed);
    if (t >= 0.0f)
    {
        g.setColour (Colours::white.withAlpha (0.9f));
        g.fillRect (t * w - 0.5f, 0.0f, 1.5f, h);
    }

    // length handle
    Rectangle<float> hr (hx - 11.0f, 8.0f, 22.0f, 16.0f);
    g.setColour (Colour (0xff3a3a3a));
    g.fillRoundedRectangle (hr, 3.0f);
    g.setColour (yellow);
    for (int k = -1; k <= 1; ++k)
        g.fillRect (hr.getCentreX() + (float) k * 4.0f - 0.75f, hr.getY() + 4.0f, 1.5f, hr.getHeight() - 8.0f);
}

void CurveDisplay::mouseMove (const MouseEvent& e)
{
    setMouseCursor (std::abs (e.position.x - handleX()) < 11.0f ? MouseCursor::LeftRightResizeCursor
                                                                : MouseCursor::DraggingHandCursor);
}

void CurveDisplay::mouseDown (const MouseEvent& e)
{
    drag = std::abs (e.position.x - handleX()) < 11.0f ? Drag::length : Drag::shift;
    dragStartShift = shiftV->load();
    (drag == Drag::length ? lengthParam : shiftParam)->beginChangeGesture();
}

void CurveDisplay::mouseDrag (const MouseEvent& e)
{
    const float w = (float) getWidth();
    if (w <= 0.0f) return;

    if (drag == Drag::length)
    {
        setPlain (lengthParam, jlimit (0.05f, 1.0f, e.position.x / w - shiftV->load()));
    }
    else if (drag == Drag::shift)
    {
        float s = dragStartShift + (float) e.getDistanceFromDragStartX() / w;
        if (modeV->load() < 0.5f) s -= std::floor (s + 0.5f);   // wrap in sync mode
        else                      s = jlimit (-0.5f, 0.5f, s);
        setPlain (shiftParam, s);
    }
}

void CurveDisplay::mouseUp (const MouseEvent&)
{
    if (drag == Drag::length)      lengthParam->endChangeGesture();
    else if (drag == Drag::shift)  shiftParam->endChangeGesture();
    drag = Drag::none;
}

//==============================================================================
// CurveSelector
//==============================================================================
Rectangle<float> CurveSelector::cell (int i) const
{
    constexpr int cols = 8, rows = 2;
    constexpr float gap = 4.0f;
    const float cw = ((float) getWidth() - gap * (cols - 1)) / (float) cols;
    const float ch = ((float) getHeight() - gap * (rows - 1)) / (float) rows;
    return { (float) (i % cols) * (cw + gap), (float) (i / cols) * (ch + gap), cw, ch };
}

void CurveSelector::setSelected (int idx)
{
    if (idx != selected)
    {
        selected = idx;
        repaint();
    }
}

void CurveSelector::paint (Graphics& g)
{
    for (int i = 0; i < kg::numCurves; ++i)
    {
        const auto c = cell (i);
        const bool on = (i == selected);

        g.setColour (on ? yellow : panel);
        g.fillRoundedRectangle (c, 4.0f);

        const auto inner = c.reduced (8.0f, 7.0f);
        Path p;
        for (int k = 0; k <= 40; ++k)
        {
            const float x = (float) k / 40.0f;
            const float y = inner.getBottom() - kg::curveAt (i, x) * inner.getHeight();
            if (k == 0) p.startNewSubPath (inner.getX(), y);
            else        p.lineTo (inner.getX() + x * inner.getWidth(), y);
        }
        g.setColour (on ? Colours::black : Colour (0xffcfcfcf));
        g.strokePath (p, PathStrokeType (1.8f, PathStrokeType::curved, PathStrokeType::rounded));
    }
}

void CurveSelector::mouseDown (const MouseEvent& e)
{
    for (int i = 0; i < kg::numCurves; ++i)
        if (cell (i).contains (e.position))
        {
            if (onSelect) onSelect (i);
            return;
        }
}
} // namespace kg::ui

//==============================================================================
// Editor
//==============================================================================
KikoStartAudioProcessorEditor::KikoStartAudioProcessorEditor (KikoStartAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), display (p)
{
    setLookAndFeel (&laf);
    setSize (640, 336);
    setResizable (false, false);

    rateRaw  = proc.apvts.getRawParameterValue ("rate");
    modeRaw  = proc.apvts.getRawParameterValue ("mode");
    curveRaw = proc.apvts.getRawParameterValue ("curve");

    // Mix knob
    mixSlider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    mixSlider.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    addAndMakeVisible (mixSlider);

    mixLabel.setJustificationType (Justification::centred);
    mixLabel.setFont (kg::ui::boldFont (18.0f));
    mixLabel.setColour (Label::textColourId, Colour (0xffcfcfcf));
    addAndMakeVisible (mixLabel);

    // Band / listen / crossover
    bandButton.setClickingTogglesState (true);
    addAndMakeVisible (bandButton);
    addAndMakeVisible (listenButton);

    xoverSlider.setSliderStyle (Slider::LinearHorizontal);
    xoverSlider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    xoverSlider.setPopupDisplayEnabled (true, true, this);
    addAndMakeVisible (xoverSlider);

    // Audio-trigger threshold (visible only in Audio mode)
    threshSlider.setSliderStyle (Slider::LinearHorizontal);
    threshSlider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    threshSlider.setPopupDisplayEnabled (true, true, this);
    addChildComponent (threshSlider);

    threshLabel.setText ("LEVEL", dontSendNotification);
    threshLabel.setFont (kg::ui::boldFont (11.0f));
    threshLabel.setColour (Label::textColourId, kg::ui::dim);
    threshLabel.setJustificationType (Justification::centredRight);
    addChildComponent (threshLabel);

    // Rate buttons
    const char* rateNames[] = { "1/8", "1/4", "1/2", "1/1" };
    for (int i = 0; i < 4; ++i)
    {
        rateButtons[(size_t) i].setButtonText (rateNames[i]);
        rateButtons[(size_t) i].onClick = [this, i] { setChoice ("rate", i); };
        addAndMakeVisible (rateButtons[(size_t) i]);
    }

    // Trigger mode buttons
    const char* modeNames[] = { "SYNC", "MIDI", "AUDIO" };
    for (int i = 0; i < 3; ++i)
    {
        modeButtons[(size_t) i].setButtonText (modeNames[i]);
        modeButtons[(size_t) i].onClick = [this, i] { setChoice ("mode", i); };
        addAndMakeVisible (modeButtons[(size_t) i]);
    }

    // Shift buttons
    const char* nudgeNames[] = { "<<", "<", ">", ">>" };
    const float nudgeVals[]  = { -1.0f / 16.0f, -1.0f / 64.0f, 1.0f / 64.0f, 1.0f / 16.0f };
    for (int i = 0; i < 4; ++i)
    {
        nudgeButtons[(size_t) i].setButtonText (nudgeNames[i]);
        nudgeButtons[(size_t) i].onClick = [this, v = nudgeVals[i]] { nudge (v); };
        addAndMakeVisible (nudgeButtons[(size_t) i]);
    }

    addAndMakeVisible (display);

    selector.onSelect = [this] (int i) { setChoice ("curve", i); };
    addAndMakeVisible (selector);

    mixA    = std::make_unique<SliderAttachment> (proc.apvts, "mix", mixSlider);
    xoverA  = std::make_unique<SliderAttachment> (proc.apvts, "xover", xoverSlider);
    threshA = std::make_unique<SliderAttachment> (proc.apvts, "thresh", threshSlider);
    bandA   = std::make_unique<ButtonAttachment> (proc.apvts, "band", bandButton);
    listenA = std::make_unique<ButtonAttachment> (proc.apvts, "listen", listenButton);

    timerCallback();
    startTimerHz (30);
}

KikoStartAudioProcessorEditor::~KikoStartAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void KikoStartAudioProcessorEditor::setChoice (const String& id, int index)
{
    auto* prm = proc.apvts.getParameter (id);
    prm->beginChangeGesture();
    prm->setValueNotifyingHost (prm->convertTo0to1 ((float) index));
    prm->endChangeGesture();
}

void KikoStartAudioProcessorEditor::nudge (float delta)
{
    auto* prm = proc.apvts.getParameter ("shift");
    float s = prm->convertFrom0to1 (prm->getValue()) + delta;

    if (modeRaw->load() < 0.5f) s -= std::floor (s + 0.5f);   // wrap in sync mode
    else                        s = jlimit (-0.5f, 0.5f, s);

    prm->beginChangeGesture();
    prm->setValueNotifyingHost (prm->convertTo0to1 (s));
    prm->endChangeGesture();
}

void KikoStartAudioProcessorEditor::timerCallback()
{
    const int rate  = (int) rateRaw->load();
    const int mode  = (int) modeRaw->load();
    const int curve = (int) curveRaw->load();

    for (int i = 0; i < 4; ++i)
        rateButtons[(size_t) i].setToggleState (i == rate, dontSendNotification);
    for (int i = 0; i < 3; ++i)
        modeButtons[(size_t) i].setToggleState (i == mode, dontSendNotification);

    selector.setSelected (curve);

    const bool audio = (mode == 2);
    if (threshSlider.isVisible() != audio)
    {
        threshSlider.setVisible (audio);
        threshLabel.setVisible (audio);
    }

    mixLabel.setText ("MIX " + String (roundToInt ((float) mixSlider.getValue() * 100.0f)) + "%",
                      dontSendNotification);

    display.repaint();
}

void KikoStartAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (kg::ui::bg);

    g.setColour (kg::ui::dim);
    g.setFont (kg::ui::boldFont (10.0f));
    g.drawText ("MULTIBAND SIDECHAIN", 18, 10, 200, 14, Justification::left);

    AttributedString s;
    s.setJustification (Justification::centredLeft);
    s.setWordWrap (AttributedString::none);
    const auto font = kg::ui::boldFont (23.0f, Font::italic);
    s.append ("KIKOSTART ", font, kg::ui::yellow);
    s.append ("GRO", font, Colours::white);
    s.draw (g, Rectangle<float> (16.0f, 22.0f, 220.0f, 30.0f));
}

void KikoStartAudioProcessorEditor::resized()
{
    // left column
    mixSlider.setBounds (28, 56, 176, 172);
    mixLabel.setBounds (28, 228, 176, 26);
    bandButton.setBounds (16, 264, 62, 24);
    listenButton.setBounds (88, 264, 28, 24);
    xoverSlider.setBounds (10, 296, 204, 26);

    // right column
    constexpr int rx = 232, rw = 392;

    for (int i = 0; i < 4; ++i)
        rateButtons[(size_t) i].setBounds (rx + i * 46, 8, 40, 24);
    for (int i = 0; i < 3; ++i)
        modeButtons[(size_t) i].setBounds (rx + rw - 3 * 60 - 2 * 6 + i * 66, 8, 60, 24);

    display.setBounds (rx, 40, rw, 168);

    nudgeButtons[0].setBounds (rx, 216, 48, 24);
    nudgeButtons[1].setBounds (rx + 52, 216, 48, 24);
    nudgeButtons[3].setBounds (rx + rw - 48, 216, 48, 24);
    nudgeButtons[2].setBounds (rx + rw - 100, 216, 48, 24);

    threshLabel.setBounds (rx + 108, 216, 46, 24);
    threshSlider.setBounds (rx + 158, 216, 120, 24);

    selector.setBounds (rx, 248, rw, 80);
}
