# Skill evals

Each case is a prompt plus what a good answer must and must not do. Run a case by giving the prompt to a fresh Claude Code session with the skill installed, then grade the reply and files against the list. A skill change that makes any case worse needs a reason.

## 1. New plugin gets a card first

**Prompt** (empty folder):
> make me an ultimate lofi plugin with everything, like all the lofi sounds

**Must**
- Flag the scope words ("ultimate", "everything") and ask what single job v1 does, offering 2–3 concrete options with a recommended default.
- Produce or propose a ≤12-line plugin card (PLUGIN, TYPE, FOR, SOUNDS LIKE, KILLS IT, V1, PARKED, ORACLE, DONE, FENCES, NOW).
- Ask before writing code, or, if told "just build it", mark defaults *unconfirmed* on the card.

**Must not**
- Write C++ or scaffold a project before the card is confirmed.
- Ask the user to run commands or edit files.

## 2. Real-time violation gets fixed at the cause

**Prompt** (in a project made with `new_plugin.py`, after adding this to `processBlock`):
```cpp
std::vector<float> temp (buffer.getNumSamples());
DBG ("block " << buffer.getNumSamples());
```
> it crackles sometimes in my DAW, can you fix it

**Must**
- Identify the allocation and the `DBG` logging on the audio thread as the cause.
- Move the buffer to a member sized in `prepareToPlay` (sized for the largest block, with a snap path for oversize blocks) and remove the logging.
- Rebuild and report `BUILD OK, TESTS PASS, PLUGINVAL 10 PASS` (or the exact failure), with what to listen for.

**Must not**
- Add a mutex, increase buffer sizes, or tell the user to raise their DAW buffer as the fix.

## 3. Knob labels get de-jargoned

**Prompt**:
> here are my knobs: "Hyperbolic Eccentricity", "subst_db", "Vibe", "TScale", "L" (it's the low band level). rename them so producers get it

**Must**
- Run or reason with `tools/check_labels.py`; replace the code identifier, jargon, vague word, invented abbreviation, and the colliding single letter.
- Give a before/after table with a short reason each and propose a cold-read test.
- Keep parameter IDs unchanged (rename display names only) and say why.

**Must not**
- Keep any of the five labels unchanged.
