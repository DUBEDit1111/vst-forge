# Workflow: from idea to plugin

Most wasted time in plugin projects comes from unclear scope, success criteria that appear after the build, and features creeping in mid-way. This loop prevents that. The person you are building for may not write code ("vibe coding"): speak in sound, make every code change yourself, and never ask them to edit or debug code.

## 1. Interview (new plugin, new major phase, or a pivot)

Skip for bug fixes and small steps inside a project that already has a card. Ask in one or two rounds. Offer concrete options with a recommended default. Do not accept "all of them" or "not sure" on questions 3–5: propose a default instead.

1. **What is it for?** One sentence: who uses it and where (your own tracks, a free giveaway, a product, a video).
2. **What does it sound like?** A reference they can point to ("like a tape delay, but darker", "like the stock compressor, but with a mix knob"). References beat adjectives.
3. **What kills it?** The result that would make them drop it ("clicks when I automate it", "sounds no different from the stock EQ").
4. **Effect or instrument? Which formats?** VST3 everywhere; add AU if they use Logic/GarageBand. Decided now, not reopened before v1.
5. **Which ONE idea ships first?** Other modes and features go on a dated parking list.
6. **What tells us it is right?** The oracle: a textbook fact to measure, a reference file, a blind A/B, a second implementation.
7. **What does done mean?** Checkable by someone else: "loads in my DAW, 3 knobs, passes pluginval 10, no clicks when automated".

When a request has two common readings ("stereo tremolo": in-phase pulse or auto-pan; "warm": darker or more saturated), ask with both options and a recommended default rather than guessing silently.

Plus 3–5 **fences**: rules the design cannot cross ("stereo in/out only", "zero latency", "no sample files", "CPU under one stock reverb").

Write a **plugin card** (≤12 lines), confirm yes/no, save it to the project's `CLAUDE.md`:

```
PLUGIN: <name>             TYPE: effect | instrument   FORMATS: VST3 [+AU]
FOR: <one sentence>        SOUNDS LIKE: <reference>
KILLS IT: <criterion>
V1 = <one idea>            PARKED: <others, with date>
ORACLE: <what proves it right>
DONE: <checkable v1>
FENCES: <3-5 rules>
NOW: <current step>
```

If they say "just build it" without answering, choose the defaults, write them on the card marked *unconfirmed*, and say so. If they later contradict the card (new format, parked feature pulled in), call it a pivot and re-ask only the changed lines.

**Input check** (one line at the top of a reply, then carry on with the best reading): flag "all of them", scope words ("ultimate", "everything": ask what job it serves), adjectives without a reference ("warmer": offer two concrete directions), and new criteria mid-build.

## 2. Design on one page (before code)

1. **One sentence.** "This plugin does X so the player can Y." If it needs "and", it is two plugins.
2. **Signal flow.** Input → stages → output; what it must never do (click, drift, alias, blow up, add latency).
3. **Controls.** ≤ 8 primary knobs, each with a plain name (see `04-parameters-ui.md`), range, curve, default, unit, and how it feels across its travel.
4. **Time.** Host tempo sync or free? What is per-sample, per-block, or on parameter change?
5. **State and feedback map.** Every delay line and filter state, what feeds back where, and the clamp/denormal/NaN guard on each loop.
6. **Proof plan.** Which measurements will show it works (see `05-testing.md`).

## 3. Build by five verbs

- **PROVE.** Build the oracle before the implementation: expected values from an independent source (`05-testing.md`). No bug fix without a reproducer.
- **GENERATE.** Start from the template with `tools/new_plugin.py`; build the untouched copy once; then change the DSP in `Source/Engine.h`. Constants in seconds and Hz; sample counts derived in `prepare()`. Tag invented numbers `PROPOSAL` so "measured" and "chosen" never blur.
- **FENCE.** Smallest correct change. No unrequested features. Never rewrite working code. Back up before editing something already released.
- **LOOP.** After every change run `scripts/build.*`: engine tests, processor tests and pluginval in one command. Fix the **first** compiler error only, then rebuild.
- **CLOSE.** End every session with something that builds and passes, or one line saying what is mid-air; update the card's NOW line.

## 4. Report

Short, in this shape:

> **What changed** (in sound terms) · **how it was verified** (commands run, numbers) · **what to try in the DAW** (which knob, what to listen for) · **what is still unverified** (ears, other OS, a specific host).

Never say "done" for something you have not run. If a step needs their ears or their DAW, say exactly that.

## 5. Session habits

- Answer first; no recap of what they just saw.
- Read the project's `CLAUDE.md` before asking for context they already gave.
- Batch independent tool calls; run long builds in the background.
- End a long session with a handoff in `CLAUDE.md`: goal, verified state, next steps with exact commands, rules, open issues.
