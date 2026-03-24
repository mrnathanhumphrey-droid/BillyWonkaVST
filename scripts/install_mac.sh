#!/bin/bash
echo "=== BW BASS v1.0.0 Installer ==="
echo ""

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# VST3
VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
mkdir -p "$VST3_DIR"
if [ -d "$SCRIPT_DIR/VST3/Groove Engine RnB.vst3" ]; then
  cp -R "$SCRIPT_DIR/VST3/Groove Engine RnB.vst3" "$VST3_DIR/"
  echo "VST3 installed to: $VST3_DIR"
fi

# AU
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$AU_DIR"
if [ -d "$SCRIPT_DIR/AU/Groove Engine RnB.component" ]; then
  cp -R "$SCRIPT_DIR/AU/Groove Engine RnB.component" "$AU_DIR/"
  echo "AU installed to: $AU_DIR"
fi

# Presets
PRESET_DIR="$HOME/Library/Application Support/BW BASS/Presets/Factory"
mkdir -p "$PRESET_DIR"
cp "$SCRIPT_DIR/Presets/Factory/"*.xml "$PRESET_DIR/" 2>/dev/null
echo "Presets installed to: $PRESET_DIR"

echo ""
echo "Done! Restart your DAW and rescan plugins."
