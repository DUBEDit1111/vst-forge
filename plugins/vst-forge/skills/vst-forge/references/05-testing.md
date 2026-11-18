# Testing and debugging: making "done" true

A test is worth keeping only if it can go **red** on a real bug and stays green through a refactor that changes nothing audible. The template ships two batteries: `Tests/EngineTests.cpp` (the DSP alone, no JUCE, seconds) and `Tests/ProcessorTests.cpp` (the whole plugin under rude-host conditions). Grow both with the plugin.

## Expected values come from an independent source

Never compute the expected value with the formula under test (it passes by construction). Strongest first:

1. A measurement of the rendered output against a physical or textbook fact (−3.01 dB at a one-pole cutoff, 6 dB/oct per pole, RT60 by Schroeder integration, an impulse at exactly N samples).
2. A second implementation by a different method or language (Python/NumPy reference vs C++, closed form vs iterative).
3. A hand-worked literal with the working in a comment (`// 10*log10(5) = 6.9897`).
4. A published table, cited.

Tag numbers that were *chosen* as `PROPOSAL` so a test never treats a choice as a fact.

## Prove every check can fail (plant a bug)

Break the code on purpose, watch the check go red, restore. A check that stays green under its planted bug is fixed or deleted. Real catches while building the template:

- A "−3 dB at 1 kHz" check could not see a missing filter prewarp (0.006 dB off). A second check at 10 kHz could (0.75 dB off).
- A "no zipper" check switched gain at a sine zero crossing, so a deleted smoother stayed green. Switching at a sine peak made it red.

How: `tools/plant_bug.py` does it for you (copies `Source/` and `Tests/` to a temp folder, applies one exact replacement, compiles and runs the engine tests, reports which checks went red, exits 1 if none did):

```bash
python <skill-dir>/tools/plant_bug.py . --file Source/Engine.h   --find "std::tan (pi * std::exp2 (logFcS) / sr)" --replace "(pi * std::exp2 (logFcS) / sr)" --label "no prewarp"
```

Never hand-edit the real source for this: a forgotten restore ships the bug. Note beside each check which planted bug it caught.

### Worked bounds

- **Zipper**: make the jump at a sine peak. A sine of peak A at f moves at most `2π·f/fs·A` per sample; allow that at the largest gain the jump reaches, ×1.05. An unsmoothed step shows a jump about the size of the whole level change.
- **Bit-transparency**: compare with `==`, not a tolerance, at neutral settings and again after moving a control away and back (catches smoothers that never land exactly). Start the comparison window soon after the move (~100 ms): a one-pole smoother without a snap-to-target eventually underflows to the exact target by itself, so a window that starts seconds later cannot see a missing snap.
- **Mono stays mono**: feed L == R and assert L == R exactly at several settings.

## What ProcessorTests should cover

Block sizes 0, 1, 3, 17, 64, 4096 and a block larger than promised; sample-rate changes (44.1 → 192 → 48 kHz); mono and stereo layouts; state save → load round-trip equals; garbage, truncated and empty state does not crash; parameter fuzz under audio stays finite and bounded; host bypass is exact when fully bypassed and click-free in transition; editor open/close churn; reported latency and tail match reality.

## Seams

Test through public boundaries: the engine's audio in/out and setters; the processor as a host sees it. Never test private members or call counts. Never mock your own DSP.

## Anti-patterns

- **Tautological**: you cannot imagine it failing.
- **Unobserved pass**: a loudness check on a silent or bypassed plugin passes without testing anything. Assert the signal was present first; report silence as INCONCLUSIVE, never PASS.
- **NaN passes "worst so far"**: a check that starts at 0 and updates on `>` never sees NaN. Write checks so only a literal `true` passes (`!(x <= worst)` to update; check all outputs are finite).
- **Synthetic-only wins**: ideas that win on sine waves often lose on real loops. Confirm on real material before claiming anything.
- **Single-metric judgement**: add a second metric that measures the failure you fear.

## The verification ladder

| Rung | Tool | Time | Catches |
|---|---|---|---|
| Engine oracle | `EngineTests` | seconds | wrong maths, instability, zipper, rate dependence |
| Processor abuse | `ProcessorTests` | seconds | block sizes, rate churn, layouts, state, garbage state, bypass, editor |
| pluginval | strictness 10, several seeds | minutes | host-contract violations, fuzz crashes, threading |
| DAW check | the user's DAW | minutes | automation, save/reopen, freeze/bounce, CPU, real-world loading |
| Ears | blind A/B | their time | everything measurements miss |

`scripts/build.*` runs the first three after every change.

### DAW checklist to hand the user

1. Load on a track with audio playing; turn every knob end to end: no clicks, no silence, no blow-ups.
2. Automate the main knob quickly: smooth, no zipper.
3. Save the project, close, reopen: sounds identical.
4. Duplicate the track: both instances independent.
5. Bypass on/off while playing: no click.
6. Change buffer size (64 → 2048) and sample rate (44.1 → 96 kHz) while playing.
7. Bounce/freeze: matches playback.

### Listening tests

Render blind A/B pairs with neutral names, put the answers in `KEY.txt`, ask them to pick before opening it. Map what they *hear* to bug classes: zipper → unsmoothed parameter; click → discontinuity (bypass, retrigger, buffer switch); drift → integrated phase or sample-rate assumption; harsh → aliasing; muddy → filter Q/curve; pumping → release too fast.

## Diagnose loop (hard bugs, wrong sound, CPU)

1. **Build a red-capable loop first.** One command, seconds, deterministic, that goes red on *this* symptom: an engine test, an offline render + measurement (RMS at f, NaN count, click detector, null against a reference), a fuzz run. Do not theorise first.
2. **Reproduce and minimise** until every remaining element matters.
3. **3–5 ranked falsifiable hypotheses** ("if X, then changing Y removes it"). Explain them in sound terms; the user often re-ranks from what they heard.
4. **Instrument** one variable at a time; never log from the audio thread (write to a preallocated buffer, dump after). For CPU: baseline first, then bisect.
5. **Fix with a regression test** that reproduces the bug pattern.
6. **Clean up**: rerun the loop, remove debug code, write down the cause that turned out true.

Intermittent bugs: raise the reproduction rate (loop ×100, vary block size, stress); 50% is debuggable, 1% is not yet.
