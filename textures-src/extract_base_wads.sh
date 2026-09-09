#!/usr/bin/env bash
# Extrahiert alle WAD3-Texturpakete aus mod-src/base verlustfrei als
# 8-Bit-Palette-BMP nach textures-src/base/<wad-name>/.
#
# Der abgeleitete Zielbaum textures-src/base wird erst nach vollstaendiger
# erfolgreicher Extraktion ersetzt. Die WAD-Quelldateien bleiben unveraendert.
#
# Nutzung:
#   ./textures-src/extract_base_wads.sh
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
SOURCE_DIR="$ROOT_DIR/mod-src/base"
TARGET_DIR="$SCRIPT_DIR/base"
TOOL="$ROOT_DIR/tools/wad_extract.py"

if [[ ! -d "$SOURCE_DIR" ]]; then
    printf 'WAD source directory not found: %s\n' "$SOURCE_DIR" >&2
    exit 1
fi

mapfile -d '' WADS < <(find "$SOURCE_DIR" -type f -iname '*.wad' -print0 | sort -z)
if [[ ${#WADS[@]} -eq 0 ]]; then
    printf 'No .wad files found in: %s\n' "$SOURCE_DIR" >&2
    exit 1
fi

WORK_DIR="$(mktemp -d "$SCRIPT_DIR/.base-wad-extract.XXXXXX")"
cleanup() {
    rm -rf -- "$WORK_DIR"
}
trap cleanup EXIT

for wad in "${WADS[@]}"; do
    relative="${wad#"$SOURCE_DIR"/}"
    wad_name="${relative%.*}"
    output="$WORK_DIR/$wad_name"

    printf 'Extracting %s\n' "$relative"
    python3 "$TOOL" "$wad" "$output"
done

# `base` is a generated tree. It is deliberately replaced only after every WAD
# has passed the extractor successfully.
rm -rf -- "$TARGET_DIR"
mv -- "$WORK_DIR" "$TARGET_DIR"
trap - EXIT

printf '\nExtracted %d WAD file(s) to %s\n' "${#WADS[@]}" "$TARGET_DIR"
