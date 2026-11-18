# VST Forge

**A Claude Code skill for vibe-coding audio plugins.** You describe the sound; Claude interviews you, scaffolds a JUCE project from a tested template, writes the DSP, names the knobs in words producers understand, tests it like a hostile DAW would, runs pluginval at strictness 10, and tells you what to listen for.

It packages a workflow that has shipped many VST3 plugins into one installable skill, so you don't have to find out the hard way that bypass clicks, `%TEMP%` breaks MSBuild, or a smoother that never lands exactly ruins transparency.

## What you get

- **A workflow**: idea interview → one-page plugin card → prove → build → gate → report → release.
- **A verified starter template** (`ToneDrive`: tone, drive, output): VST3 + Standalone on Windows/macOS/Linux, AU on macOS, header-only DSP engine, click-free crossfaded bypass, versioned state, resizable custom editor, two test batteries, and build scripts that download pluginval and run it on three seeds.
- **References**: setup per OS, JUCE/VST3 rules and build-error fixes, a DSP algorithm cheat sheet, knob-naming rules with a label checker, testing method, measured lessons, release/signing/notarization checklist, licensing.
- **Tools**: `new_plugin.py` (copy + rename the template, set plugin codes, `--synth` for instruments) and `check_labels.py` (catches jargon, code names and invented abbreviations on knobs).

## Install

**As a Claude Code plugin** (recommended):

```
/plugin marketplace add DUBEDit1111/vst-forge
/plugin install vst-forge@vst-forge
```

**As a plain skill**: copy `plugins/vst-forge/skills/vst-forge` into `~/.claude/skills/` (or your project's `.claude/skills/`).

## Quick start

1. Install the toolchain for your OS: see [00-setup.md](plugins/vst-forge/skills/vst-forge/references/00-setup.md) (Visual Studio 2022 or newer Build Tools on Windows, Xcode Command Line Tools + CMake on macOS, build-essential + JUCE's dev packages on Linux).
2. Open Claude Code in an empty folder and say something like:

   > Make me a VST3 tape-echo plugin: warm, a bit wobbly, with a mix knob. Use the vst-forge skill.

3. Answer the short interview, confirm the plugin card, and let it work. Each build ends with `BUILD OK, TESTS PASS, PLUGINVAL 10 PASS` or a precise failure Claude then fixes.
4. Load the plugin from `build/<Name>_artefacts/Release/VST3/` in your DAW (or ask Claude to install it), and run the DAW checklist it gives you.

You can also use the template without Claude:

```bash
python plugins/vst-forge/skills/vst-forge/tools/new_plugin.py ~/dev/WarmTape --name WarmTape --company "My Audio" --manufacturer Myau --code Wtap
cd ~/dev/WarmTape && ./scripts/build.sh        # Windows: powershell -ExecutionPolicy Bypass -File scripts\build.ps1
```

## Licensing: read this before you share a plugin

This repository (skill text, template, tools) is MIT-licensed. **The plugins you build link JUCE, which is AGPLv3 or commercially licensed.** Open-source your plugin under AGPLv3, or register at least the free JUCE Starter licence (closed source up to $20k revenue) before you distribute it. Details: [08-licensing.md](plugins/vst-forge/skills/vst-forge/references/08-licensing.md). Not legal advice.

## Status

- Windows: template builds, both test batteries and pluginval strictness 10 pass (verified locally).
- macOS and Linux: covered by the CI workflow in `.github/workflows/template-ci.yml`; see the latest CI run for status.
- Not covered: AAX (Pro Tools), CLAP, iOS AUv3.

## Contributing

Improvements, new measured lessons, and more templates (synth, delay, reverb) are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md).
