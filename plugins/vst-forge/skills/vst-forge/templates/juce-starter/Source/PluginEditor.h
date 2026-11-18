#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Minimal custom editor: a row of rotary knobs, one per parameter.
// Rules it demonstrates:
//  - every control is bound with an Attachment (never read/write parameters by hand)
//  - the editor owns no audio state; closing and reopening it changes nothing
//  - Attachments are declared AFTER the sliders so they are destroyed first
//  - setLookAndFeel(nullptr) in the destructor before the LookAndFeel dies
class ToneDriveEditor : public juce::AudioProcessorEditor
{
public:
    explicit ToneDriveEditor (ToneDriveProcessor&);
    ~ToneDriveEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void addKnob (Knob&, const juce::String& paramId, const juce::String& text, const juce::String& tooltip);

    ToneDriveProcessor& processor;
    juce::LookAndFeel_V4 lookAndFeel;
    juce::TooltipWindow tooltips { this, 600 };   // without one, setTooltip() text never shows
    Knob tone, drive, output;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToneDriveEditor)
};
