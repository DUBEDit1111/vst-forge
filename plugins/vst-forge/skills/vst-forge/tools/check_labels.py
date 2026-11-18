#!/usr/bin/env python3
"""Check device UI labels against the vst-forge knob-naming rules (references/04-parameters-ui.md).

Reads parameter labels from a built Max device (.amxd / .maxpat / .json), a
generator script (.mjs / .js / .py), JUCE source (.cpp / .h), or a plain text
list with one "Long | Short" (or just "Label") per line. Prints FAIL and WARN
findings and exits 1 if any FAIL was found.

The word lists live in ../references/lexicon.md (backticked terms per section).
"""
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
LEXICON = HERE.parent / "references" / "lexicon.md"  # vst-forge layout
SHORT_MAX = 15


# ---------------------------------------------------------------- lexicon
def load_lexicon(path):
    sections, current = {}, None
    suggestions = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("## "):
            current = line[3:].strip().lower()
            sections[current] = []
            continue
        if current is None:
            continue
        if current == "jargon" and line.lstrip().startswith("- ") and "→" in line:
            left, right = line.split("→", 1)
            for term in re.findall(r"`([^`]+)`", left):
                suggestions[term.lower()] = right.strip()
            continue
        if current == "approved abbreviations" and line.startswith("|"):
            cells = [c.strip() for c in line.strip("|").split("|")]
            m = re.fullmatch(r"`([^`]+)`", cells[0]) if cells else None
            if m and len(cells) > 1:
                sections[current].append((m.group(1), cells[1]))
            continue
        sections[current].extend(re.findall(r"`([^`]+)`", line))
    std = sections.get("standard words", [])
    abbrev = dict(sections.get("approved abbreviations", []))
    return {
        "words": {w.lower() for w in std},
        "phrases": {w.lower() for w in std if " " in w},
        "letters": {w for w in std if len(w) == 1},
        "abbrev": {k.lower(): v for k, v in abbrev.items()},
        "jargon": {w.lower() for w in sections.get("jargon", [])},
        "vague": {w.lower() for w in sections.get("vague words", [])},
        "suggest": suggestions,
    }


# ---------------------------------------------------------------- extraction
def from_max(text):
    """Pull longname/shortname/enum per parameter from Max JSON text."""
    params, cur = [], None
    pat = re.compile(
        r'"parameter_(longname|shortname)"\s*:\s*"((?:[^"\\]|\\.)*)"'
        r'|"parameter_enum"\s*:\s*\[([^\]]*)\]'
    )
    for m in pat.finditer(text):
        if m.group(1) == "longname":
            cur = {"long": m.group(2), "short": None, "enum": []}
            params.append(cur)
        elif m.group(1) == "shortname":
            if cur is None or cur["short"] is not None:
                cur = {"long": None, "short": m.group(2), "enum": []}
                params.append(cur)
            else:
                cur["short"] = m.group(2)
        elif m.group(3) is not None and cur is not None:
            cur["enum"] = re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(3))
    return params


def from_generator(text):
    params = from_max(text.replace("parameter_longname:", '"parameter_longname":')
                          .replace("parameter_shortname:", '"parameter_shortname":'))
    params = [p for p in params if p["long"] or p["short"]]
    if params:
        return params
    # Table rows like ["id", "Long", "Short", ...]: second and third strings.
    row = re.compile(r'\[\s*(["\'])([^"\']*)\1\s*,\s*(["\'])([^"\']+)\3\s*,\s*(["\'])([^"\']+)\5')
    out = []
    for m in row.finditer(text):
        out.append({"long": m.group(4), "short": m.group(6), "enum": []})
    return out


def from_juce(text):
    """Parameter names, plus choice items given as StringArray { "A", "B" } right
    after the name, plus label/tooltip strings set in an editor."""
    out = []
    pat = re.compile(r'ParameterID\s*\{[^}]*\}\s*,\s*"([^"]+)"(\s*,\s*juce::StringArray\s*\{([^}]*)\})?')
    for m in pat.finditer(text):
        items = re.findall(r'"([^"]+)"', m.group(3) or "")
        out.append({"long": m.group(1), "short": m.group(1), "enum": items})
    for m in re.finditer(r'(?:addKnob|addControl)\s*\([^,]+,\s*"[^"]*",\s*"([^"]+)"', text):
        if not any(p["long"] == m.group(1) for p in out):
            out.append({"long": m.group(1), "short": m.group(1), "enum": []})
    return out


def from_list(text):
    out = []
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = [p.strip() for p in line.split("|")]
        long_ = parts[0]
        short = parts[1] if len(parts) > 1 and parts[1] else long_
        out.append({"long": long_, "short": short, "enum": []})
    return out


def extract(path):
    raw = path.read_bytes().decode("utf-8", errors="replace")
    ext = path.suffix.lower()
    if ext in (".amxd", ".maxpat", ".json", ".maxhelp"):
        return from_max(raw), "built Max file"
    if ext in (".mjs", ".js", ".py", ".ts"):
        return from_generator(raw), "generator (heuristic: prefer the built .amxd)"
    if ext in (".cpp", ".h", ".hpp"):
        return from_juce(raw), "JUCE source"
    return from_list(raw), "label list"


# ---------------------------------------------------------------- checks
CODE_ID = re.compile(r"_|[a-z][A-Z]|^[a-z0-9]+$")
MASHUP = re.compile(r"^[A-Z]{1,2}[A-Z][a-z]|[a-z][A-Z]")   # TScale, MoveL
SPLIT = re.compile(r"[\s/\-]+")
LR_WORDS = {"L": ("left",), "R": ("right", "release"), "M": ("mid",), "S": ("side", "sustain"),
            "A": ("attack",), "D": ("decay",), "Q": ("q", "width", "bandwidth"),
            "X": ("x",), "Y": ("y",)}


def tokens(label):
    return [t for t in SPLIT.split(label or "") if t]


def check_token(tok, lex, long_label, alone=True):
    """Return (level, reason) or None for one word of a label."""
    low = tok.lower().strip("()[]:.,")
    if not low or re.fullmatch(r"[\d.%+#x]+", low):
        return None
    if low in lex["jargon"]:
        hint = lex["suggest"].get(low)
        return ("FAIL", f'"{tok}" is theory/project jargon'
                        + (f"; consider {hint}" if hint else "; name the sound it changes"))
    if len(tok) == 1:
        if not alone:
            return None        # variant suffix: "Allele A", "Band 3 Q"
        if tok.upper() not in lex["letters"]:
            return ("FAIL", f'single letter "{tok}" tells a producer nothing')
        meant = LR_WORDS.get(tok.upper(), ())
        if meant and not any(w in (long_label or "").lower() for w in meant):
            return ("FAIL", f'"{tok}" reads as {"/".join(meant)}; this control is something else')
        return None
    if low in lex["words"] or low in lex["abbrev"]:
        return None
    if low in lex["vague"]:
        return ("WARN", f'"{tok}" is vague; say what changes in the sound')
    if MASHUP.search(tok):
        return ("FAIL", f'"{tok}" is a mashed abbreviation; use whole words')
    if len(low) >= 3 and not re.search(r"[aeiouy]", low):
        return ("FAIL", f'"{tok}" drops vowels; use the whole word or an approved abbreviation')
    return ("WARN", f'"{tok}" is not in the lexicon; would a producer read it on sight?')


def expand(label, lex):
    """Label with approved abbreviations written out, or None if none apply."""
    out, changed = label, False
    for t in tokens(label):
        full = lex["abbrev"].get(t.lower())
        if full and full[:1].isupper() and full.lower() != t.lower():
            out = re.sub(rf"(?<![A-Za-z]){re.escape(t)}(?![A-Za-z])", full, out, count=1)
            changed = True
    return out if changed else None


def check_param(p, lex):
    found = []
    long_, short = p.get("long"), p.get("short")
    if long_ and CODE_ID.search(long_) and " " not in long_:
        found.append(("FAIL", f'longname "{long_}" is a code identifier; hosts show it in '
                              "automation lanes, Push and the mapping browser"))
    if short and len(short) > SHORT_MAX:
        found.append(("FAIL", f"shortname is {len(short)} characters; many hosts cut at {SHORT_MAX}"))
    long_is_code = bool(long_ and CODE_ID.search(long_) and " " not in long_)
    seen = set()
    for label in (short, None if long_is_code else long_):
        phrase = (label or "").lower()
        if phrase in lex["phrases"]:
            continue
        for tok in tokens(label):
            if tok.lower() in seen:
                continue
            seen.add(tok.lower())
            r = check_token(tok, lex, long_, alone=len(tokens(label)) == 1)
            if r and r not in found:
                found.append(r)
    if short:
        full = expand(short, lex)
        if full and len(full) <= SHORT_MAX:
            found.append(("WARN", f'"{full}" fits in {SHORT_MAX} characters; no need to abbreviate'))
    for item in p.get("enum", []):
        for tok in tokens(item):
            r = check_token(tok, lex, item, alone=len(tokens(item)) == 1)
            if r and r[0] == "FAIL":
                found.append((r[0], f"menu item {item!r}: " + r[1]))
    return found


def main(argv):
    if len(argv) < 2 or argv[1] in ("-h", "--help"):
        print(__doc__)
        print("usage: check_labels.py [--allow WORD]... FILE [FILE ...]   or   ... --text 'Long | Short' ...")
        return 2
    lex = load_lexicon(LEXICON)
    args = argv[1:]
    while args and args[0] == "--allow":       # the device's one signature word(s)
        if len(args) < 2:
            print("--allow needs a word"); return 2
        lex["words"].add(args[1].lower()); lex["jargon"].discard(args[1].lower())
        args = args[2:]
    argv = [argv[0]] + args
    if len(argv) < 2:
        print("no input"); return 2
    sources = []
    if argv[1] == "--text":
        sources.append(("--text", from_list("\n".join(argv[2:])), "label list"))
    else:
        for a in argv[1:]:
            path = Path(a)
            params, kind = extract(path)
            sources.append((a, params, kind))
    total_fail = total_warn = 0
    for name, params, kind in sources:
        print(f"== {name}  ({kind}, {len(params)} labels)")
        if not params:
            print("   no labels found; pass the built .amxd or a Long | Short list")
            continue
        shorts, longs = {}, {}
        for p in params:
            if p.get("short"):
                shorts.setdefault(p["short"].lower(), []).append(p)
            if p.get("long"):
                longs.setdefault(p["long"].lower(), []).append(p)
        for p in params:
            res = check_param(p, lex)
            if p.get("short") and len(shorts[p["short"].lower()]) > 1:
                res.append(("FAIL", "shortname used by more than one parameter"))
            if p.get("long") and len(longs[p["long"].lower()]) > 1:
                res.append(("FAIL", "longname used by more than one parameter; hosts need them unique"))
            head = f'{p.get("long") or "-"} | {p.get("short") or "-"}'
            for level, reason in res:
                print(f"   {level}  {head:32s}  {reason}")
                total_fail += level == "FAIL"
                total_warn += level == "WARN"
    print(f"\n{total_fail} FAIL, {total_warn} WARN. A clean run means no known pattern "
          "matched; run the cold-read test before calling the labels good.")
    return 1 if total_fail else 0


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.exit(main(sys.argv))
