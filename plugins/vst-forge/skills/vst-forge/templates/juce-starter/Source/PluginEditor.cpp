#include "PluginEditor.h"

ToneDriveEditor::ToneDriveEditor (ToneDriveProcessor& p)
    : AudioProcessorEditor (p), processor (p)
{
    lookAndFeel.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffe0b050));
    lookAndFeel.setColour (juce::Slider::thumbColourId, juce::Colour (0xfff0d080));
    setLookAndFeel (&lookAndFeel);

    // tooltip: first sentence = plain effect, direction, range (references/04-parameters-ui.md)
    addKnob (tone, "tone", "Tone", "Darkens the sound as you turn it down. 20 Hz to 20 kHz.");
    addKnob (drive, "drive", "Drive", "Adds warm distortion as you turn it up. 0 to 24 dB.");
    addKnob (output, "output", "Output", "Final volume. -24 to +12 dB.");

    setResizable (true, true);
    setResizeLimits (300, 160, 900, 480);
    setSize (420, 200);
}

ToneDriveEditor::~ToneDriveEditor()
{
    setLookAndFeel (nullptr);
}

void ToneDriveEditor::addKnob (Knob& k, const juce::String& paramId, const juce::String& text, const juce::String& tooltip)
{
    k.slider.setTooltip (tooltip);
    k.slider.setTitle (text);   // screen readers
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (k.slider);
    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.label);
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, paramId, k.slider);
}

void ToneDriveEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff141414));
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawText (JucePlugin_Name, getLocalBounds().removeFromTop (32), juce::Justification::centred);
}

void ToneDriveEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (28);
    Knob* knobs[] = { &tone, &drive, &output };
    const int w = area.getWidth() / 3;
    for (auto* k : knobs)
    {
        auto cell = area.removeFromLeft (w);
        k->label.setBounds (cell.removeFromTop (20));
        k->slider.setBounds (cell.reduced (4));
    }
}
