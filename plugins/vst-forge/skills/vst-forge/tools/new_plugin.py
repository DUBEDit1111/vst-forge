#!/usr/bin/env python3
"""Create a new JUCE plugin project from the vst-forge starter template.

    python new_plugin.py <DestFolder> --name WarmTape --company "My Audio" \
        --manufacturer Myau --code Wtap [--category Modulation] [--synth]

What it does:
  - copies templates/juce-starter to <DestFolder> (refuses to overwrite)
  - renames ToneDrive -> <name> in file contents (class names, targets, product name)
  - sets COMPANY_NAME, PLUGIN_MANUFACTURER_CODE and PLUGIN_CODE
  - --category: VST3 sub-category hosts use to file the plugin (Fx + one of
    Delay, Distortion, Dynamics, EQ, Filter, Modulation, Pitch Shift, Restoration,
    Reverb, Spatial, Analyzer, Tools); default Distortion
  - with --synth: flips the CMake flags for an instrument (you still write the voice DSP)
  - runs `git init` with a .gitignore that keeps build/ out (skip with --no-git)

Codes are permanent once you release: hosts identify your plugin by them.
  manufacturer: 4 ASCII letters/digits, first letter upper-case (e.g. Myau)
  plugin code:  4 ASCII letters/digits, exactly one upper-case letter (e.g. Wtap)
"""
import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

TEMPLATE = Path(__file__).resolve().parent.parent / "templates" / "juce-starter"
TEXT_SUFFIXES = {".txt", ".h", ".cpp", ".ps1", ".sh", ".md", ".json"}


def fail(msg: str) -> None:
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(1)


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("dest")
    ap.add_argument("--name", required=True, help="PascalCase plugin name, e.g. WarmTape")
    ap.add_argument("--company", required=True)
    ap.add_argument("--manufacturer", required=True, help="4-char manufacturer code, first letter upper-case")
    ap.add_argument("--code", required=True, help="4-char plugin code, exactly one upper-case letter")
    ap.add_argument("--category", default="Distortion", help="VST3 Fx sub-category, e.g. Modulation, Delay, Reverb")
    ap.add_argument("--synth", action="store_true", help="instrument instead of effect")
    ap.add_argument("--no-git", action="store_true", help="do not run git init")
    a = ap.parse_args()

    if not re.fullmatch(r"[A-Z][A-Za-z0-9]{1,30}", a.name):
        fail("--name must be PascalCase letters/digits, starting with an upper-case letter")
    if not re.fullmatch(r"[A-Z][A-Za-z0-9]{3}", a.manufacturer):
        fail("--manufacturer must be 4 letters/digits starting with an upper-case letter")
    if not re.fullmatch(r"[A-Za-z0-9]{4}", a.code) or sum(c.isupper() for c in a.code) != 1:
        fail("--code must be 4 letters/digits with exactly one upper-case letter")
    cats = {"Delay", "Distortion", "Dynamics", "EQ", "Filter", "Modulation", "Pitch Shift",
            "Restoration", "Reverb", "Spatial", "Analyzer", "Tools", "Generator", "Mastering"}
    if a.category not in cats:
        fail(f"--category must be one of: {', '.join(sorted(cats))}")
    if '"' in a.company:
        fail("--company must not contain double quotes")

    dest = Path(a.dest).resolve()
    if dest.exists() and any(dest.iterdir()):
        fail(f"{dest} exists and is not empty")
    shutil.copytree(TEMPLATE, dest, dirs_exist_ok=True, ignore=shutil.ignore_patterns("build", "build-*"))

    for f in dest.rglob("*"):
        if not f.is_file() or f.suffix not in TEXT_SUFFIXES:
            continue
        s = f.read_text(encoding="utf-8")
        t = s.replace("ToneDrive", a.name)
        if f.name == "CMakeLists.txt":
            t = t.replace('COMPANY_NAME "YourCompany"', f'COMPANY_NAME "{a.company}"')
            t = t.replace("PLUGIN_MANUFACTURER_CODE Ycmp", f"PLUGIN_MANUFACTURER_CODE {a.manufacturer}")
            t = t.replace("PLUGIN_CODE Tndr", f"PLUGIN_CODE {a.code}")
            t = t.replace("VST3_CATEGORIES Fx Distortion", f'VST3_CATEGORIES Fx "{a.category}"' if " " in a.category
                          else f"VST3_CATEGORIES Fx {a.category}")
            if a.synth:
                t = t.replace("IS_SYNTH FALSE", "IS_SYNTH TRUE")
                t = t.replace("NEEDS_MIDI_INPUT FALSE", "NEEDS_MIDI_INPUT TRUE")
                t = re.sub(r"VST3_CATEGORIES Fx .*", "VST3_CATEGORIES Instrument Synth", t)
                t = t.replace("AU_MAIN_TYPE kAudioUnitType_Effect", "AU_MAIN_TYPE kAudioUnitType_MusicDevice")
                t = t.replace("JUCE_MODAL_LOOPS_PERMITTED=0)", "JUCE_MODAL_LOOPS_PERMITTED=0\n    JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0)", 1)
        if t != s:
            f.write_text(t, encoding="utf-8", newline="\n")

    left = [str(f.relative_to(dest)) for f in dest.rglob("*") if f.is_file() and f.suffix in TEXT_SUFFIXES
            and "ToneDrive" in f.read_text(encoding="utf-8")]
    if left:
        fail(f"rename incomplete in: {left}")

    if not a.no_git and shutil.which("git"):
        subprocess.run(["git", "init", "-q"], cwd=dest, check=False)
        print("git repository initialised (build/ is ignored). Commit before each big change.")

    print(f"Created {a.name} at {dest}")
    if a.synth:
        print("Instrument flags set. Next: make acceptsMidi() return true, remove the input bus,")
        print("and replace Engine.h with your voice DSP (see references/03-dsp-design.md).")
    print("Next: build the untouched project once (scripts/build.ps1 or scripts/build.sh) before changing any DSP.")


if __name__ == "__main__":
    main()
