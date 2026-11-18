#include "PluginProcessor.h"
#include "PluginEditor.h"

// Parameter IDs are a public contract: never rename one after release (saved
// sessions and automation point at them). Add new ones with a higher version hint.
juce::AudioProcessorValueTreeState::ParameterLayout ToneDriveProcessor::createLayout()
{
    using FloatP = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto db = [] (float v, int) { return juce::String (v, 1) + " dB"; };
    auto hz = [] (float v, int) { return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz"
                                                      : juce::String (juce::roundToInt (v)) + " Hz"; };
    juce::NormalisableRange<float> toneRange (20.0f, 20000.0f);
    toneRange.setSkewForCentre (1000.0f);                 // log feel: 1 kHz at mid-travel

    layout.add (std::make_unique<FloatP> (juce::ParameterID { "tone", 1 }, "Tone", toneRange, 8000.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (hz)));
    layout.add (std::make_unique<FloatP> (juce::ParameterID { "drive", 1 }, "Drive",
        juce::NormalisableRange<float> (0.0f, 24.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (db)));
    layout.add (std::make_unique<FloatP> (juce::ParameterID { "output", 1 }, "Output",
        juce::NormalisableRange<float> (-24.0f, 12.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (db)));
    return layout;
}

ToneDriveProcessor::ToneDriveProcessor()
    : AudioProcessor (BusesProperties()
          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ToneDrive", createLayout())
{
    pTone = apvts.getRawParameterValue ("tone");
    pDrive = apvts.getRawParameterValue ("drive");
    pOutput = apvts.getRawParameterValue ("output");
}

void ToneDriveProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // prepare() for the new rate, then set targets from the restored parameters,
    // then reset() so the glides snap to them (a loaded session does not fade in)
    engine.prepare (sampleRate);
    engine.setCutoffHz (pTone->load());
    engine.setDriveDb (pDrive->load());
    engine.setOutputDb (pOutput->load());
    engine.reset();
    dryCopy.setSize (2, juce::jmax (samplesPerBlock, 8192), false, true, false);
    bypassStep = (float) (1.0 / (0.005 * sampleRate));
}

bool ToneDriveProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void ToneDriveProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    render (buffer, 0.0f);
}

void ToneDriveProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    render (buffer, 1.0f);
}

void ToneDriveProcessor::render (juce::AudioBuffer<float>& buffer, float target)
{
    const int n = buffer.getNumSamples();
    if (bypassMix == 0.0f && target == 0.0f) { processWet (buffer); return; }

    auto main = getBusBuffer (buffer, true, 0);
    const int chans = juce::jmin (main.getNumChannels(), 2);
    if (n > dryCopy.getNumSamples())     // host broke its block-size promise: snap, no ramp
    {
        bypassMix = target;
        if (target == 0.0f) processWet (buffer);
        return;
    }
    for (int c = 0; c < chans; ++c) dryCopy.copyFrom (c, 0, main, c, 0, n);
    processWet (buffer);
    for (int i = 0; i < n; ++i)
    {
        if (bypassMix < target)      bypassMix = juce::jmin (target, bypassMix + bypassStep);
        else if (bypassMix > target) bypassMix = juce::jmax (target, bypassMix - bypassStep);
        for (int c = 0; c < chans; ++c)
        {
            auto* o = main.getWritePointer (c);
            const float d = dryCopy.getSample (c, i);
            o[i] = bypassMix >= 1.0f ? d : o[i] + bypassMix * (d - o[i]);
        }
    }
}

void ToneDriveProcessor::processWet (juce::AudioBuffer<float>& buffer)
{
    engine.setCutoffHz (pTone->load());
    engine.setDriveDb (pDrive->load());
    engine.setOutputDb (pOutput->load());

    auto main = getBusBuffer (buffer, true, 0);
    const int n = buffer.getNumSamples();
    const int chans = main.getNumChannels();
    if (chans == 0) return;
    auto* l = main.getWritePointer (0);
    auto* r = chans > 1 ? main.getWritePointer (1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        double a = l[i], b = r ? r[i] : l[i];
        engine.processSample (a, b);
        l[i] = (float) a;
        if (r) r[i] = (float) b;
    }
}

void ToneDriveProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("stateVersion", 1, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void ToneDriveProcessor::setStateInformation (const void* data, int size)
{
    // garbage, truncated or foreign state is ignored, never a crash
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* ToneDriveProcessor::createEditor()
{
    // Custom editor: one knob per parameter, each bound by an Attachment.
    // Swap in juce::GenericAudioProcessorEditor (*this) while the DSP is still moving.
    return new ToneDriveEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ToneDriveProcessor();
}
