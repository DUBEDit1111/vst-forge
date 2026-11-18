#!/usr/bin/env python3
"""Prove an engine check can fail: plant one bug in a scratch copy, compile
EngineTests against it, and report which checks went red. The real source is
never touched.

    python plant_bug.py <project> --file Source/Engine.h \
        --find "std::tan (pi * std::exp2 (logFcS) / sr)" \
        --replace "(pi * std::exp2 (logFcS) / sr)" --label "no prewarp"

Exit 0 when at least one check went red (the bug was caught), 1 when every
check stayed green (your tests cannot see this bug: fix or add a check).
Needs a C++17 compiler: c++/clang++/g++ on macOS/Linux; on Windows, Visual
Studio 2022 (found with vswhere, no developer shell needed).
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def windows_compile(src: Path, exe: Path) -> subprocess.CompletedProcess:
    vswhere = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / \
        "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.exists():
        sys.exit("error: vswhere.exe not found; install Visual Studio (2022 or newer) with C++ tools")
    vs = subprocess.run([str(vswhere), "-latest", "-products", "*", "-requires",
                         "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
                        capture_output=True, text=True).stdout.strip()
    vcvars = Path(vs) / "VC" / "Auxiliary" / "Build" / "vcvars64.bat"
    if not vcvars.exists():
        sys.exit(f"error: {vcvars} not found")
    cmd = f'call "{vcvars}" >nul && cl /nologo /std:c++17 /O2 /EHsc /utf-8 "{src}" /Fe:"{exe}" /Fo:"{exe.parent}\\\\"'
    return subprocess.run(cmd, shell=True, capture_output=True, text=True)


def unix_compile(src: Path, exe: Path) -> subprocess.CompletedProcess:
    cxx = os.environ.get("CXX") or shutil.which("c++") or shutil.which("clang++") or shutil.which("g++")
    if not cxx:
        sys.exit("error: no C++ compiler found (set CXX)")
    return subprocess.run([cxx, "-std=c++17", "-O2", str(src), "-o", str(exe)], capture_output=True, text=True)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("project", help="plugin project folder (has Source/ and Tests/)")
    ap.add_argument("--file", required=True, help="file to break, relative to the project, e.g. Source/Engine.h")
    ap.add_argument("--find", required=True, help="exact text to replace (must occur exactly once)")
    ap.add_argument("--replace", required=True, help="the broken version")
    ap.add_argument("--label", default="planted bug")
    ap.add_argument("--tests", default="Tests/EngineTests.cpp")
    a = ap.parse_args()

    proj = Path(a.project).resolve()
    with tempfile.TemporaryDirectory(prefix="plant_bug_") as tmp:
        work = Path(tmp)
        shutil.copytree(proj / "Source", work / "Source")
        shutil.copytree(proj / "Tests", work / "Tests")
        target = work / a.file
        text = target.read_text(encoding="utf-8")
        n = text.count(a.find)
        if n != 1:
            sys.exit(f"error: --find text occurs {n} times in {a.file}; it must occur exactly once")
        target.write_text(text.replace(a.find, a.replace), encoding="utf-8")

        src = work / a.tests
        exe = work / ("EngineTests.exe" if os.name == "nt" else "EngineTests")
        c = windows_compile(src, exe) if os.name == "nt" else unix_compile(src, exe)
        if c.returncode != 0:
            print((c.stdout + c.stderr)[-3000:])
            sys.exit(f"error: the bugged copy did not compile; plant a bug that compiles")
        r = subprocess.run([str(exe)], capture_output=True, text=True, timeout=600)

    red = [l for l in r.stdout.splitlines() if l.startswith("FAIL")]
    print(f"== {a.label}: {a.file}")
    if red or r.returncode != 0:
        for l in red:
            print("  caught by  " + l)
        if not red:
            print(f"  caught (tests exited {r.returncode})")
        return 0
    print("  NOT CAUGHT: every check stayed green. Add or sharpen a check, then re-run.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
