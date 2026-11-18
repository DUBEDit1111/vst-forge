// Engine.h - the DSP, header-only, no JUCE. Template: Tone (one-pole TPT lowpass),
// Drive (tanh with 1/sqrt(drive) level compensation) and Output gain.
//
// Rules this file demonstrates (keep them when you replace the DSP):
//  - prepare(sr) computes everything rate-dependent; reset() clears state and snaps
//    the smoothers to their targets so a freshly loaded preset does not fade in.
//  - setters only store targets; every audible value glides per sample (20 ms).
//  - constants live in seconds and Hz; sample counts are derived in prepare().
//  - no allocation, locks or I/O; safe to call from the audio thread after prepare().
//  - processSample() is the seam the tests drive (EngineTests.cpp).
#pragma once
#include <algorithm>
#include <cmath>

class ToneDriveEngine
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        glide = std::exp (-1.0 / (0.020 * sr));   // 20 ms one-pole smoother
        reset();
    }

    void reset()
    {
        z[0] = z[1] = 0.0;
        logFcS = logFcT;
        driveS = driveT;
        gainS = gainT;
    }

    void setCutoffHz (double hz)  { logFcT = std::log2 (std::clamp (hz, 10.0, 0.45 * sr)); }
    void setDriveDb (double db)   { driveT = std::pow (10.0, std::clamp (db, 0.0, 36.0) / 20.0); }
    void setOutputDb (double db)  { gainT = std::pow (10.0, std::clamp (db, -60.0, 12.0) / 20.0); }

    void processSample (double& left, double& right)
    {
        // glide in log-frequency so a cutoff sweep sounds even across octaves
        logFcS = logFcT + (logFcS - logFcT) * glide;
        driveS = driveT + (driveS - driveT) * glide;
        gainS = gainT + (gainS - gainT) * glide;
        // land exactly on targets: Drive 0 dB exactly linear, Output 0 dB exactly unity,
        // even after an automation move (transparency survives automation)
        if (std::abs (logFcS - logFcT) < 1e-9) logFcS = logFcT;
        if (std::abs (driveS - driveT) < 1e-9) driveS = driveT;
        if (std::abs (gainS - gainT) < 1e-9) gainS = gainT;

        const double g = std::tan (pi * std::exp2 (logFcS) / sr);  // prewarped: -3.01 dB lands on fc
        const double G = g / (1.0 + g);
        const double comp = 1.0 / std::sqrt (driveS);

        double* io[2] = { &left, &right };
        for (int c = 0; c < 2; ++c)
        {
            double x = *io[c];
            if (driveS > 1.0) x = std::tanh (driveS * x) * comp;
            const double v = (x - z[c]) * G;                        // TPT one-pole lowpass
            const double y = v + z[c];
            z[c] = y + v;
            *io[c] = y * gainS;
        }
    }

private:
    static constexpr double pi = 3.14159265358979323846;
    double sr = 48000.0, glide = 0.0;
    double z[2] {};
    double logFcT = std::log2 (20000.0), logFcS = std::log2 (20000.0);
    double driveT = 1.0, driveS = 1.0, gainT = 1.0, gainS = 1.0;
};
