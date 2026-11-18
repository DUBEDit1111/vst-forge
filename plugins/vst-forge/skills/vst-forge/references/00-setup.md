# Setup: toolchain per OS

Do this once per machine. Then build the untouched starter template before writing any DSP: if the skeleton does not build, nothing you add will.

## What you need everywhere

- **Git** (CMake downloads JUCE with it).
- **CMake 3.25 or newer.**
- **A C++17 compiler** (below, per OS).
- **Python 3.9+** for `tools/new_plugin.py` and `tools/check_labels.py`.
- **pluginval** is downloaded automatically by `scripts/get-pluginval.*` the first time you build. Set the `PLUGINVAL` environment variable to use your own copy.
- About 2 GB of disk for JUCE and the build tree. The first configure clones JUCE (a few minutes); later builds reuse it.

## Windows 10/11

1. Install **Visual Studio 2022** (Community is free) or **Build Tools for Visual Studio 2022** with the *Desktop development with C++* workload. It includes a CMake you can use.
2. Install Git: `winget install Git.Git`. Optional standalone CMake: `winget install Kitware.CMake`.
3. Build: `powershell -ExecutionPolicy Bypass -File scripts\build.ps1`

Traps:
- Keep the project in a **short path outside `%TEMP%`** (for example `C:\dev\MyPlugin`). MSBuild fails with `FTK1011`, then CMake says "No CMAKE_CXX_COMPILER could be found", under long Temp paths.
- **Close your DAW** before rebuilding a plugin it has loaded. The DAW locks the `.vst3` (`LNK1104 cannot open file`). A DAW that "closed" can leave a hidden process holding the lock: check Task Manager.
- Installing to `C:\Program Files\Common Files\VST3` needs an admin shell. Most DAWs also let you add a custom VST3 folder, which needs no admin rights.
- Write source files as UTF-8. Windows PowerShell 5.1 `Set-Content` writes the system code page and mangles non-ASCII characters; use your editor or Python.

## macOS (12+, Apple Silicon or Intel)

1. `xcode-select --install` (Command Line Tools) or full Xcode from the App Store.
2. `brew install cmake ninja` (Homebrew from brew.sh).
3. Build: `./scripts/build.sh`

The template builds a universal binary (arm64 + x86_64) and adds the **AU** format, which Logic and GarageBand need. Install copies to `~/Library/Audio/Plug-Ins/VST3` and `~/Library/Audio/Plug-Ins/Components`. After installing an AU, run `killall -9 AudioComponentRegistrar` or log out if a host does not see it, then validate with `auval -a`.

Unsigned plugins you build yourself load on your own Mac. Plugins you give to other people must be signed and notarized: see `07-release.md`.

## Linux (Ubuntu/Debian; adapt package names for other distros)

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git pkg-config \
  libasound2-dev libjack-jackd2-dev ladspa-sdk libcurl4-openssl-dev \
  libfreetype-dev libfontconfig1-dev libx11-dev libxcomposite-dev \
  libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
  libglu1-mesa-dev mesa-common-dev xvfb
./scripts/build.sh
```

pluginval needs a display; on a headless machine or CI use `xvfb-run ./scripts/build.sh`. Install copies to `~/.vst3`. Reaper, Bitwig and Ardour load VST3 on Linux.

## Offline or faster configure

Point CMake at a local JUCE checkout of the same tag as `CMakeLists.txt`:
- Windows: `scripts\build.ps1 -Juce C:\dev\JUCE`
- macOS/Linux: `JUCE_DIR=~/dev/JUCE ./scripts/build.sh`

## Check it worked

The build script ends with `*** BUILD OK, TESTS PASS, PLUGINVAL 10 PASS ***` and prints the plugin path. Load the Standalone app from `build/<Name>_artefacts/Release/Standalone/` to hear it without a DAW.
