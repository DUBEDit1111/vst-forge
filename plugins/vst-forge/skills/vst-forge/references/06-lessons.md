# Lessons ledger: measured findings from real builds

Each line was measured on a real plugin, not guessed. Use them as priors; re-measure when your context differs. Add your own with the number that proved them.

## Dynamics and limiting

- **Low-band ripple**: a compressor's detector follows the waveform of steady bass and distorts it. Taking `max(|x|, |analytic(lowpass 250 Hz)|)` (a Hilbert allpass pair gives the analytic signal) cut 40 Hz THD from −38 to −75 dB with no loudness cost and no latency. The lowpass must be Butterworth; LR4 droop lets ripple back in.
- **Stereo link by correlation** `k = √max(ρ, 0)` cut opposite-channel pumping from 1.92 to 0.80 dB with no drift on panned mono.
- **True-peak** limiting needs 8× oversampled detection (4× let +0.28 dB through on noise). Never judge safety with a meter that shares the detector's filter.
- **Crest-adaptive release** (hold up to 10 ms, release scaled by a 300 ms crest factor) cut bass distortion on sustained notes by a large margin while drum recovery changed ≤ 10%.
- Compare dynamics designs at **equal loudness**, with drive matched by bisection. At equal loudness many single-band designs land within ~1.3 dB of each other on sub-bass distortion: some limits are structural.
- Ideas that failed when measured: tonality-adaptive release (kick + pad reads as tonal, slows, pumps); spectral cut before a limiter (loses loudness as fast as peak); adaptive phase rotation (≤ 0.2 dB headroom, smears transients).

## Spectral processing

- Detect resonances against a **line through spectral peaks**, not raw bins vs a mean.
- Log-width window means must be **1/k weighted**; a plain mean sags on sloped spectra and over-cuts.
- The resonance reference needs a **guard band** (ring mean excluding nearby bins) or a resonance raises its own reference and only gets half cut.
- Analyse the **power sum of channels plus per-channel coherence**, never mid only.

## Linear phase

- Pure linear-phase resonant filters pre-ring audibly on kicks (+20 to +33 dB over the pre-masking line). A hybrid that gives the FIR only the part of each section that decays within ~5 ms, with a minimum-phase stage for the rest, fixed it.

## Maths and "special numbers"

- Curve shapes matter: a sech² release gave 15–20 dB less bass distortion than exponential at equal time-to-1/10.
- Picking delay or LFO ratios from φ, √2 or other "special" constants made no measurable difference; mutually prime rounding of lengths is what matters.
- Do not let marketing copy claim more than the maths: measure before you write it.

## Host and platform behaviour

- Hosts bypass a VST3 through `processBlockBypassed`; JUCE's default bypass clicked until crossfaded.
- DAWs lock loaded `.vst3` files; a hidden DAW process can keep the lock after the window closes.
- `String::formatted` with `%s` garbles on Windows (wide strings).
- JUCE's generated `moduleinfo.json` can contain trailing commas: strip them before parsing it with a strict JSON parser.
- MSBuild fails (`FTK1011`, then "No CMAKE_CXX_COMPILER") under long `%TEMP%` paths.
- Git Bash rewrites `/flags` into paths: run `dumpbin /dependents` with `MSYS_NO_PATHCONV=1`.
- Windows PowerShell 5.1 `Set-Content` mangles UTF-8 without BOM; write files with an editor or Python.
- In PowerShell, `& pluginval.exe` does not wait (GUI subsystem) and `$LASTEXITCODE` is stale: use `Start-Process -Wait -PassThru` (the template's build script does).
- A static `juce::Image` (or other JUCE object with static lifetime) can hang DLL unload in some hosts; keep JUCE objects as members, not statics.

## Process

- The most expensive failure: losing the source/generator for a shipped plugin. Keep everything in git from day one.
- Derived files (checksums, zips, version strings in READMEs) go stale: rebuild them with a script, never by hand.
- Two sessions editing the same files at once corrupt each other's work: one writer per file set.
