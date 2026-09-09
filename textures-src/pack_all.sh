#!/usr/bin/env bash
# Packt jeden Unterordner unter textures-src/rewolf/<variante>/ (BMP-Dateien)
# wieder zu einem WAD3-Texturpaket, Ergebnis in textures-src/output/.
#
# Nutzt tools/wad_pack.py (reines Python). Texturgroessen muessen durch 8
# teilbar sein (Standard-GoldSrc-Einschraenkung); nicht passende Texturen
# werden vom Tool uebersprungen und aufgelistet.
#
# Nutzung:
#   ./pack_all.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RE_PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
TOOL="$RE_PROJECT_DIR/tools/wad_pack.py"
OUT_DIR="$SCRIPT_DIR/output"
mkdir -p "$OUT_DIR"

shopt -s nullglob
count=0
for dir in "$SCRIPT_DIR"/rewolf/*/; do
    name="$(basename "$dir")"
    out="$OUT_DIR/$name.wad"
    python3 "$TOOL" "$dir" "$out"
    count=$((count + 1))
done

echo
echo "Fertig: $count Pakete -> $OUT_DIR/"
