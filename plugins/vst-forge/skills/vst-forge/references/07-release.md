# Release and packaging

A release goes to strangers' computers: it must load without extra installs, must not click, and every file in the package must match what you tested.

## Before any release

- [ ] Labels pass `tools/check_labels.py` with no FAIL, and a cold-read test was done (`04-parameters-ui.md`).
- [ ] Name checked: a trademark knock-out search (USPTO, EUIPO; classes 9/41/42) and a web search for audio products with the same or similar name.
- [ ] Licences settled (`08-licensing.md`): JUCE tier or AGPL compliance, VST3 SDK MIT notice shipped, third-party code and fonts listed.
- [ ] Release build; pluginval strictness 10 on several seeds and once with GUI tests; the DAW checklist in `05-testing.md` passed.
- [ ] Plugin and manufacturer codes final (they can never change after release).
- [ ] Version number set in `project(... VERSION x.y.z)`; hosts read it.
- [ ] Tested on a second machine that never had your dev tools installed.

### Windows

- [ ] Static runtime verified: `dumpbin /dependents` lists no `MSVCP140.dll` / `VCRUNTIME140.dll`.
- [ ] Code signing optional for plugins (an unsigned installer `.exe` triggers SmartScreen warnings; a zip of the `.vst3` mostly does not).

### macOS

- [ ] Universal binary (arm64 + x86_64): `lipo -info <...>.vst3/Contents/MacOS/<Name>`.
- [ ] Signed with a Developer ID Application certificate (paid Apple Developer account) and **notarized**, or Gatekeeper reports "damaged" on other Macs:

```bash
codesign --force --deep --options runtime --timestamp \
  --sign "Developer ID Application: <Your Name> (<TEAMID>)" MyPlugin.vst3 MyPlugin.component
ditto -c -k --keepParent MyPlugin.vst3 MyPlugin-vst3.zip
xcrun notarytool submit MyPlugin-vst3.zip --keychain-profile <profile> --wait
xcrun stapler staple MyPlugin.vst3
```

  Repeat for the `.component`. A `.pkg` installer needs a Developer ID Installer certificate (`productsign`), then notarize and staple the `.pkg`.
- [ ] `auval -v aufx <code> <manu>` passes for the AU (`aumu` for instruments).

### Linux

- [ ] Built on the oldest distro you support (glibc is forward-compatible only), or in a container of it.
- [ ] Ship the `.vst3` folder in a `.tar.gz` with install instructions (`~/.vst3`).

## Package layout (flat)

```
<Product> v1.0/
  <Product>.vst3        (and <Product>.component on macOS)
  README.txt            what it does, install steps, controls in sound terms, version
  LICENSE.txt           your EULA or licence, plus third-party notices (VST3 SDK MIT, JUCE if required)
  CHECKSUMS.sha256      LF line endings so `sha256sum -c` passes
<Product>-<os>.zip
```

Build the package with a script: copy from the build output, write README/LICENSE/CHECKSUMS, zip, then **re-open the zip** and verify the inner hashes and that the shipped binary equals what you tested. Keep the previous package before rebuilding.

## Install steps to give users

- **Windows**: close the DAW, copy `<Product>.vst3` to `C:\Program Files\Common Files\VST3` (or the DAW's custom VST3 folder), reopen, rescan plug-ins.
- **macOS**: copy `.vst3` to `~/Library/Audio/Plug-Ins/VST3` and `.component` to `~/Library/Audio/Plug-Ins/Components`, reopen the DAW (Logic rescans AUs at start).
- **Linux**: copy `.vst3` to `~/.vst3`, rescan.

## Store copy

Describe what it does in sound terms, with an audio demo. No claims you have not measured; no medical or "healing frequency" claims (consumer-protection law in most countries). If you say "zero latency" or "CPU-light", have the number.

## Automate it

Once the plugin is stable, a CI workflow (see the repo's `.github/workflows/template-ci.yml`) can build Windows, macOS and Linux on every push and attach artefacts to a tagged release. Keep signing secrets in the CI secret store, never in the repo.
