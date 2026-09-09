#!/usr/bin/env bash
# Extrahiert alle "rewolf"-WAD3-Texturpakete (Retail + ggf. weitere
# Sprachvarianten/Builds) als BMP-Dateien nach textures-src/rewolf/<variante>/.
#
# Nutzt tools/wad_extract.py (reines Python, verlustfrei -- siehe
# Roundtrip-Test in tools-setup.md).
#
# Nutzung:
#   ./extract_all.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RE_PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
GAME_DIR="$(dirname "$RE_PROJECT_DIR")"
TOOL="$RE_PROJECT_DIR/tools/wad_extract.py"

declare -A WADS=(
    [retail]="$GAME_DIR/Gunman-ENG-GER/rewolf/rewolf.WAD"
    [retail_german]="$GAME_DIR/Gunman-ENG-GER/rewolf_german/rewolf.WAD"
    [e3beta]="$GAME_DIR/Gunman-E3/rewolf.WAD"
)

for variant in "${!WADS[@]}"; do
    wad="${WADS[$variant]}"
    if [ ! -f "$wad" ]; then
        echo "Uebersprungen ($variant): $wad nicht gefunden."
        continue
    fi
    out="$SCRIPT_DIR/rewolf/$variant"
    mkdir -p "$out"
    python3 "$TOOL" "$wad" "$out"
done

echo
echo "Fertig. Texturen liegen unter $SCRIPT_DIR/rewolf/<variante>/*.bmp"
echo "Hinweis: 'retail' und 'retail_german' sind haeufig identisch (gleiche"
echo "MD5 des rewolf.WAD in beiden Sprachordnern) -- vor Bearbeitung ggf. mit"
echo "'diff -rq' pruefen, um doppelte Arbeit zu vermeiden."
