// ProcessorTests.cpp - host-abuse battery for the whole plugin (processor + editor).
// Everything a rude host does: absurd block sizes, rate churn, layout demands,
// garbage state, parameter fuzz under audio, bypass toggling, editor churn.
// Exit 0 = all green. Runs in seconds, no DAW needed.
#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include <random>

static int fails = 0;
static void check (const char* name, bool ok)
{
    std::printf ("%s: %s\n", ok ? "PASS" : "FAIL", name);
    if (! ok) fails = 1;
}

static bool finiteBuffer (const juce::AudioBuffer<float>& b)
{
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (! std::isfinite (b.getSample (c, i))) return false;
    return true;
}

static void fillSine (juce::AudioBuffer<float>& b, double& ph, double f, double sr, float amp)
{
    for (int i = 0; i < b.getNumSamples(); ++i, ph += f / sr)
        for (int c = 0; c < b.getNumChannels(); ++c)
            b.setSample (c, i, amp * (float) std::sin (juce::MathConstants<double>::twoPi * ph));
}

// EDIT WHEN YOU CHANGE PARAMETERS: put the plugin in a setting where its output
// clearly differs from its input, so the bypass test below measures something.
// Returns false if the parameter is missing (the test then fails loudly instead of crashing).
static bool makeWetDifferFromDry (ToneDriveProcessor& p)
{
    auto* tone = p.apvts.getParameter ("tone");
    if (tone == nullptr) return false;
    tone->setValueNotifyingHost (0.0f);   // 20 Hz lowpass: wet and dry differ a lot
    return true;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;   // message manager for the editor test
    std::mt19937 rng (7);
    using CS = juce::AudioChannelSet;

    // 1. block sizes 0/1/3/17/64/4096 (and back) with a sine: all finite
    {
        ToneDriveProcessor p;
        p.setPlayConfigDetails (2, 2, 48000.0, 4096);
        p.prepareToPlay (48000.0, 4096);
        juce::MidiBuffer midi; bool ok = true; double ph = 0;
        for (int bs : { 0, 1, 3, 17, 64, 4096, 1, 0, 4096 })
        {
            juce::AudioBuffer<float> buf (2, juce::jmax (bs, 1));
            buf.clear();
            juce::AudioBuffer<float> view (buf.getArrayOfWritePointers(), 2, bs);
            fillSine (view, ph, 440.0, 48000.0, 0.5f);
            p.processBlock (view, midi);
            ok = ok && finiteBuffer (view);
        }
        check ("block sizes 0/1/3/17/64/4096 finite", ok);
    }

    // 2. sample-rate churn: prepare at 44.1k/48k/96k/192k back to back
    {
        ToneDriveProcessor p; juce::MidiBuffer midi; bool ok = true; double ph = 0;
        for (double sr : { 44100.0, 48000.0, 96000.0, 192000.0, 44100.0 })
        {
            p.setPlayConfigDetails (2, 2, sr, 512);
            p.prepareToPlay (sr, 512);
            juce::AudioBuffer<float> buf (2, 512);
            fillSine (buf, ph, 1000.0, sr, 0.5f);
            p.processBlock (buf, midi);
            ok = ok && finiteBuffer (buf);
        }
        check ("sample-rate churn finite", ok);
    }

    // 3. layouts: mono and stereo accepted, quad and mono->stereo refused
    {
        ToneDriveProcessor p;
        auto mk = [&] (CS in, CS out) {
            juce::AudioProcessor::BusesLayout l;
            l.inputBuses.add (in); l.outputBuses.add (out);
            return p.checkBusesLayoutSupported (l);
        };
        check ("stereo/stereo accepted", mk (CS::stereo(), CS::stereo()));
        check ("mono/mono accepted", mk (CS::mono(), CS::mono()));
        check ("quad refused", ! mk (CS::quadraphonic(), CS::quadraphonic()));
        check ("mono->stereo refused", ! mk (CS::mono(), CS::stereo()));
    }

    // 4. state round-trip: randomise every parameter, save, scramble, restore, compare
    {
        ToneDriveProcessor p;
        std::uniform_real_distribution<float> u (0.0f, 1.0f);
        for (auto* prm : p.getParameters()) prm->setValueNotifyingHost (u (rng));
        std::vector<float> want;
        for (auto* prm : p.getParameters()) want.push_back (prm->getValue());
        juce::MemoryBlock mb; p.getStateInformation (mb);
        for (auto* prm : p.getParameters()) prm->setValueNotifyingHost (u (rng));
        p.setStateInformation (mb.getData(), (int) mb.getSize());
        bool ok = true; size_t k = 0;
        for (auto* prm : p.getParameters()) ok = ok && std::abs (prm->getValue() - want[k++]) < 1e-6f;
        check ("state round-trip exact", ok);
    }

    // 5. garbage state: random bytes, truncated state, empty - never crash, params stay in range
    {
        ToneDriveProcessor p;
        juce::MemoryBlock good; p.getStateInformation (good);
        std::vector<char> junk (777);
        for (auto& c : junk) c = (char) (rng() & 0xff);
        p.setStateInformation (junk.data(), (int) junk.size());
        p.setStateInformation (good.getData(), (int) good.getSize() / 2);
        p.setStateInformation (nullptr, 0);
        bool sane = true;
        for (auto* prm : p.getParameters()) sane = sane && prm->getValue() >= 0.0f && prm->getValue() <= 1.0f;
        check ("garbage state harmless", sane);
    }

    // 6. parameter fuzz under audio: new random values every block, 300 blocks, finite
    {
        ToneDriveProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 256); p.prepareToPlay (48000.0, 256);
        std::uniform_real_distribution<float> u (0.0f, 1.0f);
        juce::MidiBuffer midi; bool ok = true; double ph = 0;
        for (int b = 0; b < 300; ++b)
        {
            for (auto* prm : p.getParameters()) prm->setValueNotifyingHost (u (rng));
            juce::AudioBuffer<float> buf (2, 256);
            fillSine (buf, ph, 80.0, 48000.0, 1.0f);
            p.processBlock (buf, midi);
            ok = ok && finiteBuffer (buf);
        }
        check ("parameter fuzz under audio finite (300 blocks)", ok);
    }

    // 7. bypass: after the 5 ms ramp the output IS the input, and toggling never clicks
    {
        ToneDriveProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 64); p.prepareToPlay (48000.0, 64);
        check ("bypass test: makeWetDifferFromDry() found its parameter", makeWetDifferFromDry (p));
        juce::MidiBuffer midi; double ph = 0; float prev = 0.0f, worst = 0.0f, wetDiff = 0.0f; bool exact = true;
        for (int b = 0; b < 400; ++b)
        {
            juce::AudioBuffer<float> buf (2, 64), in (2, 64);
            fillSine (buf, ph, 1000.0, 48000.0, 0.5f);
            in.makeCopyOf (buf);
            const bool bypassed = (b / 50) % 2 == 1;
            if (bypassed) p.processBlockBypassed (buf, midi); else p.processBlock (buf, midi);
            for (int i = 0; i < 64; ++i) { worst = juce::jmax (worst, std::abs (buf.getSample (0, i) - prev)); prev = buf.getSample (0, i); }
            if (! bypassed && b % 50 >= 10)
                for (int i = 0; i < 64; ++i) wetDiff = juce::jmax (wetDiff, std::abs (buf.getSample (0, i) - in.getSample (0, i)));
            if (bypassed && b % 50 >= 10)
                for (int i = 0; i < 64; ++i) exact = exact && buf.getSample (0, i) == in.getSample (0, i);
        }
        // a 0.5-peak 1 kHz sine moves at most 0.0655 per sample; a hard switch would jump ~0.5
        // unobserved-pass guard: if wet == dry, the two bypass checks prove nothing
        check ("bypass test: processed output differs from input", wetDiff > 0.05f);
        check ("bypassed output equals input exactly", exact);
        check ("bypass toggling is click-free", worst < 0.09f);
    }

    // 8. editor churn: create and destroy 5 times
    {
        ToneDriveProcessor p; bool ok = true;
        for (int k = 0; k < 5; ++k)
        {
            std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
            ok = ok && ed != nullptr;
        }
        check ("editor create/destroy x5", ok);
    }

    // 9. latency and tail reported honestly
    {
        ToneDriveProcessor p;
        check ("zero latency reported", p.getLatencySamples() == 0);
        check ("zero tail reported", p.getTailLengthSeconds() == 0.0);
    }

    std::printf (fails ? "PROCESSOR TESTS: FAIL\n" : "PROCESSOR TESTS: OK\n");
    return fails;
}
