#!/usr/bin/env python3
"""Fail if the repo contains leaks: absolute home-directory paths, e-mail
addresses, or extra private terms you list in a local file.

    python scripts/scrub_check.py                 # built-in patterns
    python scripts/scrub_check.py --terms .scrub-terms.txt   # plus your own (one per line, not committed)

Exit 1 on any hit.
"""
import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SKIP_DIRS = {".git", "build", "__pycache__", "node_modules"}
TEXT = {".md", ".py", ".txt", ".h", ".cpp", ".ps1", ".sh", ".json", ".yml", ".yaml", ""}

PATTERNS = [
    (r"[A-Za-z]:\\Users\\(?!<)[A-Za-z0-9._-]+", "absolute Windows home path"),
    (r"/Users/(?!<)[A-Za-z0-9._-]+/", "absolute macOS home path"),
    (r"/home/(?!<|runner\b)[A-Za-z0-9._-]+/", "absolute Linux home path"),
    (r"[A-Za-z0-9._%+-]+@(?!anthropic\.com)[A-Za-z0-9.-]+\.(com|net|org|io|co|uk|de)\b", "e-mail address"),
]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--terms", help="file with extra case-insensitive terms, one per line")
    a = ap.parse_args()
    pats = [(re.compile(p), why) for p, why in PATTERNS]
    if a.terms:
        for t in Path(a.terms).read_text(encoding="utf-8").splitlines():
            t = t.strip()
            if t and not t.startswith("#"):
                pats.append((re.compile(re.escape(t), re.I), f"private term '{t}'"))

    me = Path(__file__).resolve()
    hits = 0
    for f in ROOT.rglob("*"):
        if f.is_dir() or any(p in SKIP_DIRS for p in f.parts) or f.suffix not in TEXT or f.resolve() == me:
            continue
        if a.terms and f.resolve() == Path(a.terms).resolve():
            continue
        try:
            lines = f.read_text(encoding="utf-8").splitlines()
        except UnicodeDecodeError:
            continue
        for n, line in enumerate(lines, 1):
            for rx, why in pats:
                if rx.search(line):
                    print(f"{f.relative_to(ROOT)}:{n}: {why}: {line.strip()[:120]}")
                    hits += 1
    print(f"{hits} hit(s)" if hits else "scrub check clean")
    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main())
