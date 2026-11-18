#!/usr/bin/env bash
# get-pluginval.sh - prints the path to pluginval, downloading the latest release from
# https://github.com/Tracktion/pluginval into ~/.local/share/pluginval if needed.
# Set the PLUGINVAL environment variable to use your own copy.
set -euo pipefail
if [[ -n "${PLUGINVAL:-}" && -x "$PLUGINVAL" ]]; then echo "$PLUGINVAL"; exit 0; fi
DIR=~/.local/share/pluginval
if [[ "$(uname)" == "Darwin" ]]; then ASSET=pluginval_macOS.zip; EXE=$DIR/pluginval.app/Contents/MacOS/pluginval
else ASSET=pluginval_Linux.zip; EXE=$DIR/pluginval; fi
if [[ ! -x "$EXE" ]]; then
  mkdir -p "$DIR"
  echo "Downloading pluginval..." >&2
  curl -fsSL -o "$DIR/$ASSET" "https://github.com/Tracktion/pluginval/releases/latest/download/$ASSET"
  unzip -oq "$DIR/$ASSET" -d "$DIR" && rm "$DIR/$ASSET"
  chmod +x "$EXE"
fi
echo "$EXE"
