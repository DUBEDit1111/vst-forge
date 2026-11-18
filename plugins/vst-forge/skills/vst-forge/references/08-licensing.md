# Licensing you must know before you share or sell a plugin

Not legal advice. Licence terms change: check the linked pages on the day you release, and ask a lawyer for anything with money attached.

## JUCE (the framework the template uses)

JUCE is dual-licensed (https://juce.com/get-juce/, checked November 2026):

- **AGPLv3**: free. If you distribute a plugin built with JUCE under AGPLv3, you must release your plugin's complete source code under AGPLv3 too.
- **JUCE licence tiers** (closed source allowed):

| Tier | Price | Revenue limit |
|---|---|---|
| Starter | free | up to $20,000 |
| Indie | $40/user/month, or $800 once | up to $300,000 |
| Pro | $175/user/month, or $3,500 once | unlimited |
| Educational | free | for education |

You must hold the licence for as long as you distribute closed-source binaries that contain JUCE. Building plugins for yourself, never distributed outside your organisation, does not trigger AGPL distribution duties.

**Rule of thumb**: giving away a free plugin with source on GitHub → AGPLv3 is fine. Closed source (free or paid) → register the Starter licence at minimum, and move up a tier as revenue grows.

## Steinberg VST3 SDK

The VST3 SDK (bundled inside JUCE) is MIT-licensed (https://github.com/steinbergmedia/vst3sdk). Ship its copyright and permission notice in your `LICENSE.txt` or a third-party notices file.

The "VST" name and logo are Steinberg trademarks. Saying your plugin is "a VST3 plugin" is common practice; using the VST logo has its own usage guidelines in the SDK documentation. Read them before putting the logo on a product page.

## This repo

The skill text, the starter template and the tools in this repository are MIT-licensed: use them in any project, open or closed. The template downloads JUCE at build time; JUCE's own licence (above) governs the plugins you build with it.

## Other things that need a licence check

- **Third-party DSP code**: GPL code in a closed-source plugin is not allowed. MIT/BSD/zlib code is fine with its notice shipped. Note where every borrowed snippet came from.
- **Fonts**: many free fonts (SIL Open Font License) can be embedded; some "free for personal use" fonts cannot be in a sold product.
- **Samples and impulse responses** shipped with the plugin: you need redistribution rights, not just usage rights.
- **Plugin names**: search trademarks before you print the name anywhere (`07-release.md`).
- **AU on macOS**: no extra licence, but distribution to others needs Apple Developer ID signing and notarization (paid developer account).
- **AAX (Pro Tools)**: needs a free Avid developer account, the AAX SDK, and PACE signing for release builds. Not covered by this template.
