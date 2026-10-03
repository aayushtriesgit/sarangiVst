#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// Brass knobs with a saffron value arc.
class SarangiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SarangiLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
};

class SarangiEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SarangiEditor (SarangiProcessor&);
    ~SarangiEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawStrings (juce::Graphics&, juce::Rectangle<float> area);
    void drawBow (juce::Graphics&, juce::Rectangle<float> area);
    juce::String currentSwar() const;

    SarangiProcessor& proc;
    SarangiLookAndFeel lnf;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label, hint;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    std::array<Knob, 6> knobs;
    juce::ComboBox saBox;
    juce::Label saLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> saAttach;

    juce::Rectangle<float> stringArea;
    float level = 0.0f, hz = 0.0f, phase = 0.0f, bowPos = 0.5f, bowDir = 1.0f, bowAlpha = 0.0f;
    bool bowing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SarangiEditor)
};
