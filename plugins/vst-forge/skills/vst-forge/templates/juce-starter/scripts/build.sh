#!/usr/bin/env bash
# build.sh - macOS / Linux: configure, build, run both test batteries, run pluginval, optionally install.
#   ./scripts/build.sh              # build + test + validate
#   ./scripts/build.sh --install    # ...then copy to your user plug-in folder
# Env: JUCE_DIR=<local JUCE checkout> skips the git download; SEEDS=3 pluginval seeds.
# Linux needs JUCE's dev packages first (see references/00-setup.md) and a display
# for pluginval (on a headless box: xvfb-run ./scripts/build.sh).
set -euo pipefail
cd "$(dirname "$0")/.."
NAME=$(sed -n 's/^juce_add_plugin(\([A-Za-z0-9_]*\).*/\1/p' CMakeLists.txt | head -1)
SEEDS=${SEEDS:-3}
INSTALL=0; [[ "${1:-}" == "--install" ]] && INSTALL=1

if [[ ! -f build/CMakeCache.txt ]]; then
  ARGS=(-S . -B build -DCMAKE_BUILD_TYPE=Release)
  [[ -n "${JUCE_DIR:-}" ]] && ARGS+=(-DFETCHCONTENT_SOURCE_DIR_JUCE="$JUCE_DIR")
  command -v ninja >/dev/null && ARGS+=(-G Ninja)
  cmake "${ARGS[@]}"
fi
TARGETS=(EngineTests ProcessorTests "${NAME}_VST3" "${NAME}_Standalone")
[[ "$(uname)" == "Darwin" ]] && TARGETS+=("${NAME}_AU")
cmake --build build --config Release --parallel --target "${TARGETS[@]}"

find_one() { find build -type f -path "*$1" -not -path "*/_deps/*" | head -1; }
echo; echo "== Engine tests";    "$(find_one /EngineTests)"
echo; echo "== Processor tests"; "$(find_one /ProcessorTests_artefacts/*ProcessorTests)"

VST=$(find build -type d -name "${NAME}.vst3" -path "*_artefacts*" | head -1)
PV=$(./scripts/get-pluginval.sh)
echo; echo "== pluginval strictness 10, $SEEDS seeds"
for i in $(seq 1 "$SEEDS"); do
  SEED=$(printf '0x%x' $((0x1111 * i)))
  "$PV" --strictness-level 10 --validate-in-process --skip-gui-tests --seed "$SEED" --validate "$VST"
  echo "seed $SEED passed"
done

echo; echo "*** BUILD OK, TESTS PASS, PLUGINVAL 10 PASS ***"; echo "Plugin: $VST"
if [[ $INSTALL == 1 ]]; then
  if [[ "$(uname)" == "Darwin" ]]; then
    DEST=~/Library/Audio/Plug-Ins/VST3
    AU=$(find build -type d -name "${NAME}.component" -path "*_artefacts*" | head -1)
    if [[ -n "$AU" ]]; then
      mkdir -p ~/Library/Audio/Plug-Ins/Components
      rm -rf ~/Library/Audio/Plug-Ins/Components/"${NAME}.component"
      cp -R "$AU" ~/Library/Audio/Plug-Ins/Components/
    fi
  else
    DEST=~/.vst3
  fi
  mkdir -p "$DEST"; rm -rf "$DEST/${NAME}.vst3"; cp -R "$VST" "$DEST/"
  echo "Installed to $DEST - rescan plug-ins in your DAW."
fi
