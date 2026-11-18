#pragma once
#include <JuceHeader.h>
#include "Engine.h"

class ToneDriveProcessor : public juce::AudioProcessor
{
public:
    ToneDriveProcessor();
    ~ToneDriveProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    ToneDriveEngine engine;

    // Host bypass: the engine keeps running and the output crossfades to the dry
    // signal over 5 ms, so switching never clicks and the filter state stays warm.
    // Fully bypassed output is the input, bit for bit.
    void render (juce::AudioBuffer<float>& buffer, float target);
    void processWet (juce::AudioBuffer<float>& buffer);
    juce::AudioBuffer<float> dryCopy;
    float bypassMix = 0.0f;              // 0 = processed, 1 = dry
    float bypassStep = 1.0f / 240.0f;

    // cached raw pointers: never look a parameter up by ID on the audio thread
    std::atomic<float>* pTone {};
    std::atomic<float>* pDrive {};
    std::atomic<float>* pOutput {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToneDriveProcessor)
};
