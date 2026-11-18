# Parameters, labels and UI

## Naming knobs (the label rules)

A label is for a producer with a session open, not for the person who wrote the DSP. They glance at a knob for under three seconds and must know **what it changes in the sound** and **which way is more**. The maths behind a control belongs in the manual and the second sentence of the tooltip, never on the knob.

1. **Name the sound, not the mechanism.** Complete "Turning this up makes the sound ___." The label comes from the blank.
2. **Standard name wins.** If it does what a familiar control does, use the familiar name ("Threshold", "Mix", "Tone"), even if the DSP inside is new. See `lexicon.md`.
3. **New control, everyday words.** One or two plain words for the audible result ("Tail Length", "Time Spread"). No coined words or metaphors that need a manual.
4. **No invented abbreviations.** If the full word fits, use it. If not, only abbreviations from `lexicon.md`'s approved list. Never drop vowels or mash words ("TScale", "Proc").
5. **No code on the face.** Parameter *names* show in hosts' automation lanes. `env_atk`, `subst_db` are identifiers; IDs stay internal, names are full words in Title Case, unique.
6. **No colliding letters.** L/R/M/S mean left/right/mid/side; A/D/S/R mean an envelope; Q is filter Q. Nothing else gets a single letter.
7. **No vibe words alone.** "Magic", "Vibe", "Energy", "Character", bare "Amount" say nothing. Say what changes: "Grit", "Brightness", "Movement", "Drive".
8. **One signature name per plugin, at most**, and its tooltip opens with the plain effect.
9. **Menus name the result.** "Smooth / Punchy / Hard", not "Mode 1 / Mode 2".
10. **Tooltip order.** First sentence: plain effect, direction, range, unit ("Raises quiet parts by up to 24 dB. 0 = off.").
11. **Units in the value, not the name.** "Attack" showing "12 ms", not "Attack ms".

### Check labels

```bash
python tools/check_labels.py Source/PluginProcessor.cpp      # JUCE source
python tools/check_labels.py labels.txt                      # one "Long | Short" per line
python tools/check_labels.py --allow Bloom Source/PluginProcessor.cpp   # rule 8 signature word
```

FAIL = must fix (code identifier, jargon, invented abbreviation, duplicate, colliding letter). WARN = read and decide. A clean run means no known pattern matched, not that the labels are good. Then do a **cold-read test**: give a fresh agent (a subagent on a different model if you can) only the labels and one line on the plugin type, with this brief:

> You are a producer who knows your DAW's stock plugins well. This is a [plugin type]. For each knob label, say in a few words what turning it up does to the sound, or "no idea". Do not guess politely.

Every "no idea" or wrong guess is a rename. Use two readers; keep a name only when both read it right.

JUCE hosts key automation by `ParameterID`, so renaming the display name is safe; never change the ID after release.

## Parameter design

- ≤ 8 primary controls. Macros over micro-knobs.
- Each parameter: name, range, skew, default, unit text, and how it feels across its travel. Frequencies and times on a log skew (`setSkewForCentre`).
- Defaults should sound good and safe on first load (no full drive, no 100% feedback).
- Output/makeup gain so A/B comparisons can be level-matched.
- A Mix (dry/wet) knob on most effects; keep the dry path phase-aligned with any latency.

## Choice parameters (menus, switches)

```cpp
// createLayout(): items name the result you hear (rule 9); the checker reads them
layout.add (std::make_unique<juce::AudioParameterChoice> (
    juce::ParameterID { "shape", 1 }, "Shape", juce::StringArray { "Smooth", "Choppy" }, 0));

// processor: cache once, read the index per block
std::atomic<float>* pShape = apvts.getRawParameterValue ("shape");   // holds the index as float
const int shape = (int) pShape->load();

// editor: a ComboBox bound with a ComboBoxAttachment (declare the attachment after the box)
juce::ComboBox shapeBox;
std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment;
// in the constructor, BEFORE creating the attachment, fill the items in the same order:
shapeBox.addItemList (processor.apvts.getParameter ("shape")->getAllValueStrings(), 1);
shapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "shape", shapeBox);
```

Switching a choice changes the sound discretely: crossfade old and new paths over 10–20 ms in the engine instead of jumping. On/off switches: `juce::AudioParameterBool` + `ToggleButton` + `ButtonAttachment`.

## Editor (GUI)

The template ships `Source/PluginEditor.*`: a resizable row of rotary knobs, each bound with a `SliderAttachment`. Grow it rather than starting over.

- One Attachment per control (`SliderAttachment`, `ButtonAttachment`, `ComboBoxAttachment`). Declare the attachment **after** its control so it is destroyed first.
- The editor owns no audio state. Opening and closing it must change nothing; hosts create and destroy editors constantly (the processor tests do too).
- Meters and scopes: the audio thread writes to `std::atomic<float>` or a lock-free FIFO; a 30 Hz `juce::Timer` in the editor reads and repaints. Never call the editor from the audio thread.
- Look and feel: subclass `juce::LookAndFeel_V4`, override `drawRotarySlider` etc. Call `setLookAndFeel(nullptr)` in the editor destructor.
- Fonts and images: `juce_add_binary_data(<Name>Assets SOURCES assets/logo.png assets/Font.ttf)` then link `<Name>Assets`. Check font licences (`08-licensing.md`).
- Layout in `resized()` with `juce::Rectangle` slicing (`removeFromTop`, `removeFromLeft`) or `juce::FlexBox`/`Grid`. Support resizing with `setResizable` + `setResizeLimits`, or a fixed aspect ratio.
- Text that will not fit: `label.setMinimumHorizontalScale(0.8f)` before falling back to a shorter word.
- Tooltips need a `juce::TooltipWindow` member in the editor (the template has one); without it `setTooltip` text never shows.
- Accessibility: give controls a title (`slider.setTitle`) so screen readers name them.
- While the DSP is still changing, `juce::GenericAudioProcessorEditor(*this)` is a zero-effort UI; switch to the custom editor once the parameter list is stable.
