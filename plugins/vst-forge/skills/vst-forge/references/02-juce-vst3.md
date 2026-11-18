# JUCE VST3 plugins (JUCE 9, CMake)

Starter: `templates/juce-starter/` (create a copy with `tools/new_plugin.py`). It has a CMake project with a static Windows runtime, a header-only DSP engine, a processor with click-free crossfaded bypass, a custom editor, engine oracle tests, host-abuse tests, and build scripts that run everything plus pluginval at strictness 10.

## Facts

- JUCE 9 (template pins a tag; bump it deliberately and rebuild everything). C++17, CMake ≥ 3.25.
- Formats: VST3 on all three OSes, AU on macOS (Logic, GarageBand), Standalone for quick listening. VST2 is dead (no new SDK licences). JUCE has no built-in CLAP: use `free-audio/clap-juce-extensions` if you need it; Ableton Live does not host CLAP.
- Licence: see `08-licensing.md` before you give or sell a build.
- pluginval: `pluginval --strictness-level 10 --validate-in-process [--skip-gui-tests] --seed 0x1111 --validate <x.vst3>`. Ship only at 10, on several seeds.
- Install folders: Windows `C:\Program Files\Common Files\VST3` (admin); macOS `~/Library/Audio/Plug-Ins/VST3` and `.../Components` (AU); Linux `~/.vst3`. Then rescan plug-ins in the DAW.

## Build

The scripts do this for you. By hand:

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64          # Windows (VS 2026: "Visual Studio 18 2026")
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release         # macOS / Linux
cmake --build build --config Release --target EngineTests ProcessorTests <Name>_VST3
```

- Build the untouched skeleton before changing anything. Read the **first** compiler error only.
- **Static MSVC runtime** (in the template): without it the `.vst3` needs `MSVCP140.dll`/`VCRUNTIME140.dll` and fails to load on PCs without the VC++ redistributable. Verify with `dumpbin /dependents <...>.vst3\Contents\x86_64-win\<Name>.vst3` (in Git Bash prefix `MSYS_NO_PATHCONV=1`).
- Codes: manufacturer 4 chars, first upper-case; plugin code 4 chars, exactly one upper-case. **Permanent after release**: hosts identify the plugin by them.
- Instruments: `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `VST3_CATEGORIES Instrument Synth`, `AU_MAIN_TYPE kAudioUnitType_MusicDevice`, `JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0`; output bus only. MIDI effects: `IS_MIDI_EFFECT TRUE`.
- Ship Release builds only. A Debug build is slow and can fail to load in some hosts.

## Non-negotiable rules

**Real-time safety.** Inside `processBlock` never allocate (`new`, `push_back`, `juce::String`, `std::function` capture), lock a mutex, do I/O (`DBG` and file logging included), touch the GUI, or throw. Pre-allocate in `prepareToPlay`. Share data with the GUI through `std::atomic` or `juce::AbstractFifo`. First line of every audio callback: `juce::ScopedNoDenormals`.

**Parameters.** `AudioProcessorValueTreeState` with one `createLayout()` and `ParameterID{"id", 1}`. IDs never change after release (saved sessions and automation point at them); display names can. Log skew for Hz and ms (`setSkewForCentre`). Value-to-text functions with units. Cache raw pointers once (`getRawParameterValue`); never look up by ID on the audio thread. Smooth everything audible (gain ~5 ms, cutoff ~20 ms in log-frequency, delay time 50–200 ms); never smooth discrete choices, crossfade them. A new feature defaults to off and is **bit-exact with the old path when off**, so old sessions sound unchanged.

**Bypass.** Hosts bypass a VST3 by calling `processBlockBypassed`. Override it and crossfade over ~5 ms with the engine still running (state stays warm, no click). Fully bypassed output equals the input bit for bit. If you override `getBypassParameter()`, you get `processBlock` and must honour bypass yourself. JUCE's default bypass clicks.

**Blocks and rates.** Recompute everything rate-dependent in `prepareToPlay`. Never assume a block size: 0 and 1 happen, and hosts sometimes send more samples than they promised (snap instead of ramping when `n` exceeds your buffers). Support 44.1–192 kHz. Oversample nonlinear stages (`juce::dsp::Oversampling`) and report it with `setLatencySamples`. Accept mono and stereo in `isBusesLayoutSupported`; refuse the rest.

**State.** `apvts.copyState()` plus a `stateVersion` property, written as XML. On load: check the tag, ignore garbage or truncated data (never crash), migrate old versions, `replaceState`. In `prepareToPlay`, set the engine targets from the restored params and then reset the smoothers, so a loaded session does not fade in.

**Code shape.** The DSP is a header-only class with no JUCE (`prepare`, `reset`, setters, `processSample`): testable without a host and reusable. Keep its interface small and test only through it. One class per file pair, `juce::` explicit.

## Patterns

- **Host tempo**: `getPlayHead()->getPosition()->getBpm()` (optionals) once per block; keep the last good value. Synced time = beats × 60 / bpm. Offer sync and free modes. Tempo-synced LFO phase comes from the host's PPQ position each block, never integrated.
- **Sidechain**: `.withInput("Sidechain", stereo(), false)`; `getBusBuffer(buffer, true, 1)` may have 0 channels: fall back to the main input.
- **Synth**: `juce::Synthesiser` + voices (PolyBLEP oscillator → SVF → ADSR), voice steal with a 5 ms fade.
- **MIDI effect**: iterate `for (const auto meta : midi)`, build a new `MidiBuffer`, `swapWith`; all-notes-off on transport stop.
- **GUI**: one Attachment per control; a 30 Hz `Timer` reading atomics for meters; scope data through a lock-free FIFO; `juce::FontOptions` (the old `Font(float)` constructor is deprecated); custom `LookAndFeel_V4`; `setLookAndFeel(nullptr)` in the editor destructor; images and fonts via `juce_add_binary_data`.
- **Presets**: XML of the state tree; factory presets in BinaryData; user presets in `userApplicationDataDirectory/<Company>/<Product>/Presets`.
- **Momentary buttons** (Randomize) re-roll state no snapshot can restore: exclude them from state round-trip tests.

## Build errors → fixes

| Symptom | Cause / fix |
|---|---|
| `LNK1104 cannot open ...vst3` | A host still has it loaded. Close the DAW (check for hidden processes). |
| `FTK1011`, "No CMAKE_CXX_COMPILER could be found" | Build dir under `%TEMP%` or path too long. Move to `C:\dev\...`. |
| `jassert` in APVTS at startup | Duplicate or invalid parameter ID. |
| Plugin not listed in DAW | Wrong folder, 32-bit host, pluginval fails, or DAW needs a rescan. |
| pluginval "value out of range" | Range interval or skew wrong. |
| NaN / inf in output | Unclamped cutoff near Nyquist, denormals, or divide by zero in a feedback loop. |
| Crash at odd block sizes | Allocation in `processBlock` or a size assumption. |
| Garbled text on Windows | `String::formatted` with `%s` takes wide strings: use `juce::String` concatenation. |
| macOS: "damaged, can't be opened" | Unsigned/quarantined download: sign and notarize (`07-release.md`). |
| Linux: missing `alsa/asoundlib.h`, `X11/...` | Install the dev packages in `00-setup.md`. |

## Verification ladder (before a DAW sees it)

1. `EngineTests` (no JUCE, seconds): measured responses against textbook facts, stability fuzz, zipper bound, bounded output, sample-rate independence.
2. `ProcessorTests` (whole plugin): block sizes 0/1/3/17/64/4096, rate churn, layouts, state round-trip, garbage state, parameter fuzz under audio, bypass exact and click-free, editor open/close churn, honest latency/tail.
3. pluginval strictness 10 on several seeds, then once without `--skip-gui-tests`.
4. In the DAW: audio passes, automation is smooth, save/reopen sounds identical, duplicated instances are independent, freeze/bounce equals playback, buffer 64→2048 and 44.1→96 kHz without clicks, bypass click-free, CPU against a comparable stock plugin.
5. Ears, blind A/B where it matters.
