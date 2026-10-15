#pragma once

#include "PluginProcessor.h"

namespace kg::ui
{
inline const juce::Colour yellow { 0xffffe526 };
inline const juce::Colour bg     { 0xff1b1b1b };
inline const juce::Colour panel  { 0xff2a2a2a };
inline const juce::Colour screen { 0xff141414 };
inline const juce::Colour dim    { 0xff8a8a8a };

//==============================================================================
class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos,
                           float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;

    int getSliderThumbRadius (juce::Slider&) override { return 9; }

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
};

//==============================================================================
class HeadphoneButton : public juce::Button
{
public:
    HeadphoneButton() : juce::Button ("listen") { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

//==============================================================================
// Shows the waveform of the input, the volume curve, the play-head and a drag handle
// for the curve length. Dragging anywhere else moves the curve (shift).
class CurveDisplay : public juce::Component
{
public:
    explicit CurveDisplay (KikoStartAudioProcessor&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    float handleX() const;
    static void setPlain (juce::RangedAudioParameter*, float plainValue);

    KikoStartAudioProcessor& proc;
    juce::RangedAudioParameter* shiftParam  = nullptr;
    juce::RangedAudioParameter* lengthParam = nullptr;
    std::atomic<float>* shiftV  = nullptr;
    std::atomic<float>* lengthV = nullptr;
    std::atomic<float>* curveV  = nullptr;
    std::atomic<float>* modeV   = nullptr;

    enum class Drag { none, length, shift };
    Drag drag = Drag::none;
    float dragStartShift = 0.0f;
};

//==============================================================================
class CurveSelector : public juce::Component
{
public:
    std::function<void (int)> onSelect;

    void setSelected (int idx);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> cell (int i) const;
    int selected = -1;
};
} // namespace kg::ui

//==============================================================================
class KikoStartAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit KikoStartAudioProcessorEditor (KikoStartAudioProcessor&);
    ~KikoStartAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setChoice (const juce::String& paramId, int index);
    void nudge (float delta);

    KikoStartAudioProcessor& proc;
    kg::ui::LookAndFeel laf;

    juce::Slider mixSlider, xoverSlider, threshSlider;
    juce::Label mixLabel, threshLabel;
    juce::TextButton bandButton { "BAND" };
    kg::ui::HeadphoneButton listenButton;

    std::array<juce::TextButton, 4> rateButtons;
    std::array<juce::TextButton, 3> modeButtons;
    std::array<juce::TextButton, 4> nudgeButtons;

    kg::ui::CurveDisplay display;
    kg::ui::CurveSelector selector;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> mixA, xoverA, threshA;
    std::unique_ptr<ButtonAttachment> bandA, listenA;

    std::atomic<float>* rateRaw  = nullptr;
    std::atomic<float>* modeRaw  = nullptr;
    std::atomic<float>* curveRaw = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KikoStartAudioProcessorEditor)
};
