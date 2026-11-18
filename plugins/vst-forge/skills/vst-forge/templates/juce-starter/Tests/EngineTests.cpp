// EngineTests.cpp - the DSP oracle. Dependency-free (no JUCE), builds and runs in seconds.
// Every expected value comes from an independent source: a textbook fact measured on
// the rendered output, or a worked literal with its working in the comment. Never
// recompute the engine's own formula here: that test could not fail.
//
// Proven red-capable (2026-10-08): with the 20 ms glide disabled, test 5 fails; with
// the prewarp removed (g = pi*fc/sr), test 1b fails. Test 1 alone could NOT catch the
// missing prewarp (0.006 dB off at 1 kHz): that is why every check gets a planted bug.
#include "../Source/Engine.h"
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

static int fails = 0;
static void check (const char* name, bool ok, double got = NAN)
{
    std::printf ("%s: %s", ok ? "PASS" : "FAIL", name);
    if (! std::isnan (got)) std::printf ("  (got %.4f)", got);
    std::printf ("\n");
    if (! ok) fails = 1;
}

static const double SR = 48000.0, PI = 3.14159265358979323846;

// sine RMS gain in dB through the engine, measured after 0.5 s of settling
static double gainDb (double fc, double f, double driveDb = 0.0, double outDb = 0.0)
{
    ToneDriveEngine e;
    e.prepare (SR);
    e.setCutoffHz (fc); e.setDriveDb (driveDb); e.setOutputDb (outDb);
    e.reset();
    double in2 = 0, out2 = 0;
    const int settle = (int) (0.5 * SR), len = (int) SR;
    for (int i = 0; i < settle + len; ++i)
    {
        const double x = 0.1 * std::sin (2 * PI * f * i / SR);
        double l = x, r = x;
        e.processSample (l, r);
        if (i >= settle) { in2 += x * x; out2 += l * l; }
    }
    return 10.0 * std::log10 (out2 / in2);
}

int main()
{
    // 1. one-pole lowpass: -3.01 dB at the cutoff (10*log10(2) = 3.0103)
    { const double g = gainDb (1000, 1000); check ("-3.01 dB at fc = 1 kHz", std::abs (g + 3.0103) < 0.05, g); }

    // 1b. still -3.01 dB at 10 kHz: only a prewarped (tan) coefficient holds near Nyquist
    { const double g = gainDb (10000, 10000); check ("-3.01 dB at fc = 10 kHz (prewarp)", std::abs (g + 3.0103) < 0.05, g); }

    // 2. one octave above: analog one-pole is -10*log10(1 + 2^2) = -6.99 dB
    { const double g = gainDb (1000, 2000); check ("-6.99 dB one octave above fc", std::abs (g + 6.9897) < 0.1, g); }

    // 3. passband flat: 100 Hz through a 20 kHz cutoff is ~0 dB
    { const double g = gainDb (20000, 100); check ("passband flat at 100 Hz", std::abs (g) < 0.05, g); }

    // 4. output gain: +6 dB setting = 20*log10(1.9953) = 6.00 dB measured
    { const double g = gainDb (20000, 100, 0, 6) - gainDb (20000, 100); check ("Output +6 dB measures +6.00", std::abs (g - 6.0) < 0.02, g); }

    // 5. no zipper: Output jumps -24 -> +12 dB on a 1 kHz sine of peak 0.25. A sine's
    //    steepest step is 2*pi*f/SR * peak; final peak 0.25*10^(12/20) = 0.995, so any
    //    sample-to-sample jump over 0.131*1.05 means the gain changed in one step. The jump
//    is made at a sine peak: the first version jumped at a zero crossing and stayed green
//    with the glide deleted.
    {
        ToneDriveEngine e; e.prepare (SR); e.setCutoffHz (20000); e.setOutputDb (-24); e.reset();
        double prev = 0, worst = 0;
        for (int i = 0; i < (int) SR; ++i)
        {
            if (i == 4812) e.setOutputDb (12);   // a sine PEAK (48 samples/cycle, +12 = 90 deg): at a zero crossing even an unsmoothed jump is invisible
            double l = 0.25 * std::sin (2 * PI * 1000 * i / SR), r = l;
            e.processSample (l, r);
            if (i > 0) worst = std::max (worst, std::abs (l - prev));
            prev = l;
        }
        const double bound = 2 * PI * 1000 / SR * 0.25 * std::pow (10.0, 12.0 / 20.0) * 1.05;
        check ("Output jump is smoothed (no zipper)", worst < bound, worst);
    }

    // 6. Drive is bounded: tanh(x) <= 1 and compensation 1/sqrt(d) < 1, so a +20 dB
    //    overload at Drive 24 dB, Output 0 dB can never exceed 1.0
    {
        ToneDriveEngine e; e.prepare (SR); e.setCutoffHz (20000); e.setDriveDb (24); e.reset();
        double peak = 0;
        for (int i = 0; i < (int) SR; ++i)
        {
            double l = 10.0 * std::sin (2 * PI * 100 * i / SR), r = l;
            e.processSample (l, r);
            peak = std::max (peak, std::abs (l));
        }
        check ("Drive output bounded <= 1.0 under +20 dB overload", peak <= 1.0, peak);
    }

    // 7. stability: white noise, every parameter jumped to random extremes every 64
    //    samples for 10 s; every output finite and below 4.0 (+12 dB max gain on |x| <= 1)
    {
        ToneDriveEngine e; e.prepare (SR); e.reset();
        std::mt19937 rng (1);
        std::uniform_real_distribution<double> u (0, 1), n (-1, 1);
        bool ok = true;
        for (int i = 0; i < (int) (10 * SR); ++i)
        {
            if (i % 64 == 0)
            {
                e.setCutoffHz (u (rng) < 0.3 ? (u (rng) < 0.5 ? 1.0 : 1e9) : 20 * std::pow (1000.0, u (rng)));
                e.setDriveDb (u (rng) < 0.3 ? 99 : 24 * u (rng));
                e.setOutputDb (u (rng) < 0.3 ? 99 : -60 + 72 * u (rng));
            }
            double l = n (rng), r = n (rng);
            e.processSample (l, r);
            if (! std::isfinite (l) || ! std::isfinite (r) || std::abs (l) > 4.0 || std::abs (r) > 4.0) { ok = false; break; }
        }
        check ("noise + extreme parameter jumps: finite and bounded", ok);
    }

    // 8. rate independence: -3.01 dB at fc holds at 44.1k and 192k too
    {
        bool ok = true;
        for (double sr : { 44100.0, 96000.0, 192000.0 })
        {
            ToneDriveEngine e; e.prepare (sr); e.setCutoffHz (1000); e.reset();
            double in2 = 0, out2 = 0;
            for (int i = 0; i < (int) (1.5 * sr); ++i)
            {
                const double x = 0.1 * std::sin (2 * PI * 1000 * i / sr);
                double l = x, r = x; e.processSample (l, r);
                if (i >= (int) (0.5 * sr)) { in2 += x * x; out2 += l * l; }
            }
            ok = ok && std::abs (10 * std::log10 (out2 / in2) + 3.0103) < 0.05;
        }
        check ("-3.01 dB at fc at 44.1k, 96k, 192k", ok);
    }

    // 9. transparency survives automation: Output 0 -> -6 -> 0 dB, then the output equals
    //    a fresh engine's output exactly (smoothers land on their targets, not near them)
    {
        ToneDriveEngine a, b;
        for (auto* e : { &a, &b }) { e->prepare (SR); e->setCutoffHz (20000); e->reset(); }
        bool exact = true;
        for (int i = 0; i < (int) SR; ++i)
        {
            if (i == 4800) a.setOutputDb (-6);
            if (i == 9600) a.setOutputDb (0);
            double l1 = 0.1, r1 = 0.1, l2 = 0.1, r2 = 0.1;
            a.processSample (l1, r1); b.processSample (l2, r2);
            if (i > (int) (0.75 * SR)) exact = exact && l1 == l2;
        }
        check ("Output -6 dB and back: identical to untouched engine (snap)", exact);
    }

    std::printf (fails ? "ENGINE TESTS: FAIL\n" : "ENGINE TESTS: OK\n");
    return fails;
}
