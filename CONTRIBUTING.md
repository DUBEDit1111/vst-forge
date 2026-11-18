# Contributing

Thanks for helping make plugin building easier.

## Ground rules

- **Measured, not guessed.** A new lesson in `06-lessons.md` needs the number that proved it and the condition it was measured in.
- **Templates must pass their own gate.** Any change under `templates/` must keep `scripts/build.*` green: engine tests, processor tests, pluginval strictness 10 on three seeds. CI checks Windows, macOS and Linux.
- **Every new test must be able to fail.** Plant a bug, see it go red, restore, and say in the PR which planted bug it caught.
- **Your own code only.** Do not paste code from GPL projects or copyrighted books. Algorithms and published constants are fine; cite the source.
- **Plain language.** References are read by Claude and by people who do not code. Short sentences, concrete numbers.
- Run `python scripts/scrub_check.py` before opening a PR (it catches absolute home paths and other leaks).

## Layout

```
.claude-plugin/marketplace.json            marketplace entry (one-line install)
plugins/vst-forge/.claude-plugin/plugin.json
plugins/vst-forge/skills/vst-forge/
  SKILL.md                                 hub: loop, routing table, hard rules
  references/                              loaded on demand
  templates/juce-starter/                  the verified starter project
  tools/                                   new_plugin.py, check_labels.py
evals/                                     prompts + grading notes for skill behaviour
```

## Ideas welcome

A synth template, a delay/reverb template, CLAP via clap-juce-extensions, an AAX section, a preset-browser pattern, a CI release job that signs and notarizes.
