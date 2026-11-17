#pragma once

#include "PluginProcessor.h"

namespace vt
{
namespace col
{
    inline const juce::Colour magenta { 0xffff2e93 };
    inline const juce::Colour cyan    { 0xff00e5ff };
    inline const juce::Colour lime    { 0xffb6ff3b };
    inline const juce::Colour violet  { 0xff9b5bff };
    inline const juce::Colour orange  { 0xffffa63d };
    inline const juce::Colour pink    { 0xffff6ec7 };
    inline const juce::Colour blue    { 0xff3d8bff };
}

//==============================================================================
class VibeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VibeLookAndFeel();

    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
};

//==============================================================================
// Live "what am I singing / what is it snapping to" display + lit-up scale keys
class PitchDisplay : public juce::Component,
                     public juce::SettableTooltipClient,
                     private juce::Timer
{
public:
    explicit PitchDisplay (VibeTuneAudioProcessor&);
    ~PitchDisplay() override { stopTimer(); }
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    VibeTuneAudioProcessor& proc;
    float detected = -1.0f, target = -1.0f, dispDev = 0.0f;
    std::array<bool, 12> allowed {};
};

//==============================================================================
class VibeTuneAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit VibeTuneAudioProcessorEditor (VibeTuneAudioProcessor&);
    ~VibeTuneAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
    };

    void timerCallback() override;
    void stepPreset (int delta);
    void setupKnob (Knob&, const juce::String& title, juce::Colour colour,
                    const juce::String& tip, double defaultValue, bool bipolar);

    VibeTuneAudioProcessor& proc;
    vt::VibeLookAndFeel lnf;
    juce::TooltipWindow tooltipWindow { this, 450 };

    vt::PitchDisplay display;
    juce::ComboBox presetBox, keyBox, scaleBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::ToggleButton lowLatButton { "LOW LATENCY" };
    juce::Label latencyLabel, keyCaption, scaleCaption;
    Knob speed, amount, humanize, transpose, fine, mix, gain;

    // attachments last so they are destroyed before the widgets they point at
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAtt> speedAtt, amountAtt, humanizeAtt, transposeAtt, fineAtt, mixAtt, gainAtt;
    std::unique_ptr<ComboAtt> keyAtt, scaleAtt;
    std::unique_ptr<ButtonAtt> lowLatAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibeTuneAudioProcessorEditor)
};
