---
name: vst-forge
description: End-to-end workflow for designing, building, testing and releasing VST3 (and AU) audio plugins with JUCE and CMake on Windows, macOS and Linux, aimed at people who describe the sound and let Claude write the code. Covers the idea interview and one-page plugin card, a verified starter template (DSP engine, processor with click-free bypass, custom editor, engine and host-abuse tests, pluginval at strictness 10), real-time C++ rules, DSP algorithm recipes, plain-language knob naming with a label checker, the verification ladder, packaging, signing and notarization, and licensing (JUCE, VST3 SDK). Use when the user wants to make, fix, test, port or ship a VST, VST3, AU, audio plugin, effect, synth, instrument, JUCE project or DSP algorithm, or says "vibe code a plugin", even if they never mention JUCE.
---

# VST Forge

Build audio plugins that load in every DAW, never click, and are measured before anyone listens. The user may not write code: they describe the sound, you write and verify everything.

## The loop (every plugin)

1. **Card.** New plugin or a pivot: short interview → a ≤12-line plugin card the user confirms. Bug fixes inside a carded project skip this. → `references/01-workflow.md`
2. **Scaffold.** Check the toolchain (`references/00-setup.md`), then create the project from the template and build it untouched:
   ```bash
   python <skill-dir>/tools/new_plugin.py <dest> --name MyPlugin --company "My Company" --manufacturer Myco --code Mypl --category Modulation
   ```
   (`--category` is the VST3 folder hosts file it under: Delay, Distortion, Dynamics, EQ, Filter, Modulation, Reverb, Spatial, …; `--synth` for instruments. It also runs `git init` with `build/` ignored.)
   Then run `scripts/build.ps1` (Windows) or `scripts/build.sh` (macOS/Linux) in `<dest>`. It must end with `BUILD OK, TESTS PASS, PLUGINVAL 10 PASS` before you change anything.
3. **Prove + build together.** First write the engine's public interface in `Source/Engine.h` (`prepare`, `reset`, setters, `processSample`) with a stub body, then the engine tests against it, with expected values from an independent source; then fill in the DSP until they pass. Prove each new check can fail with `tools/plant_bug.py` (breaks a scratch copy, never your source). → `references/05-testing.md`, `03-dsp-design.md`
4. **Wire it up.** Every place the template names its own parameters must change together:
   - `PluginProcessor.cpp`: `createLayout()`, the cached pointers in the constructor, `prepareToPlay()` and `processWet()` setters.
   - `PluginEditor.*`: one control per parameter (choice parameters use a `ComboBox`, see `04-parameters-ui.md`), tooltips.
   - `Tests/ProcessorTests.cpp`: `makeWetDifferFromDry()` at the top must set one of *your* parameters so the output differs from the input.
   - `Tests/EngineTests.cpp`: the engine class name and setters.
   Name every control by the plain-language rules and run `tools/check_labels.py Source/PluginProcessor.cpp`. → `references/02-juce-vst3.md`, `04-parameters-ui.md`
5. **Gate.** Re-run the build script after every change (engine tests → processor tests → pluginval 10 on 3 seeds). Read the **first** error only.
6. **Report.** What changed in sound terms · how it was verified (commands, numbers) · what to try in the DAW · what is still unverified.
7. **Release.** Checklist, packaging, signing/notarization, licences. → `references/07-release.md`, `08-licensing.md`

## Route by task

| Task | Load |
|---|---|
| New idea, "build me a plugin that…", scope, pivot | `01-workflow.md` |
| Install tools, first build fails, OS-specific trouble | `00-setup.md` |
| JUCE, CMake, parameters, bypass, state, build errors | `02-juce-vst3.md` |
| Choosing or tuning an algorithm (filter, delay, reverb, saturation, dynamics, synth) | `03-dsp-design.md`, then `06-lessons.md` |
| Knob names, tooltips, editor layout, meters, look and feel | `04-parameters-ui.md` + `lexicon.md` |
| Tests, "is it right?", a click, a crash, CPU, a bug | `05-testing.md` |
| Packaging, signing, notarization, store copy | `07-release.md` |
| Licences (JUCE, VST3 SDK, fonts, third-party code) | `08-licensing.md` |

## Hard rules (break none)

1. **Speak sound, not code.** Never ask the user to edit or debug code or run commands you could run. One recommended default per decision.
2. **Never say done for something not run.** Silence in a test is INCONCLUSIVE, not PASS. Report real numbers.
3. **Independent expected values.** Never compute a test's expected value with the code under test. Plant a bug once to prove each new check can fail.
4. **Audio thread:** no allocation, locks, I/O, logging, GUI calls or exceptions in `processBlock`. Pre-allocate in `prepareToPlay`. `juce::ScopedNoDenormals` first.
5. **Smooth every audible control** (gain ~5 ms, cutoff ~20 ms in log-frequency); crossfade discrete switches; snap smoothers to restored values at load.
6. **Parameter IDs and plugin/manufacturer codes are permanent** once released. New features default off and are bit-exact with the old sound when off.
7. **Host contract:** crossfaded `processBlockBypassed`, any block size (0, 1, huge), 44.1–192 kHz, mono and stereo, garbage state ignored, pluginval strictness 10 on several seeds.
8. **Windows builds** use the static MSVC runtime (template does) and Release config; keep build trees in a short path outside `%TEMP%`.
9. **Labels are plain producer words** (`04-parameters-ui.md`); run `tools/check_labels.py` and a cold-read test before release.
10. **Smallest correct change.** No unrequested features, no rewrites of working code, everything in git.
11. **Wins must hold on real audio at equal loudness**, not only on test tones.
12. **No unmeasured claims** in names, UI or store copy (no medical claims, no "analog-modelled" unless it is).

## What is in this skill

- `templates/juce-starter/`: an effect ("ToneDrive": tone, drive, output). CMake fetches JUCE; VST3 + Standalone everywhere, AU on macOS; header-only engine; processor with crossfaded bypass and versioned state; resizable custom editor; `EngineTests` (no JUCE, each check proven able to fail) and `ProcessorTests` (host-abuse battery); `scripts/build.ps1|.sh` (build, both batteries, pluginval auto-download, optional install).
- `tools/new_plugin.py`: copies and renames the template, sets codes, `--synth` for instruments.
- `tools/plant_bug.py`: plants one bug in a scratch copy, compiles the engine tests, reports which checks went red (exit 1 if none did).
- `tools/check_labels.py` + `references/lexicon.md`: label checker for parameter names, menu items and editor labels.

To make an instrument: `new_plugin.py --synth`, then make `acceptsMidi()` return true, drop the input bus, and replace the engine with voices (see `02-juce-vst3.md` Patterns and `03-dsp-design.md` Oscillators). Keep both test batteries and grow them with the plugin.
