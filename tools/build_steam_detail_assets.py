#!/usr/bin/env python3
"""Generate GoldSrc detail-texture assets from BSP texture references.

GoldSrc detail textures are overlays, not HD texture replacements. The script
creates neutral 128px detail tiles in gfx/detail and one map detail file per
BSP. Transparent, water and sky textures are deliberately excluded.
"""
from __future__ import annotations

import argparse
import shutil
import struct
import subprocess
import tempfile
from collections import defaultdict
from pathlib import Path

TEXTURE_LUMP = 2
LUMP_COUNT = 15
EXCLUDED_WADS = {"cached", "decals", "fonts", "gfx", "tempdecal"}


def bsp_texture_names(path: Path) -> list[str]:
    data = path.read_bytes()
    if len(data) < 4 + LUMP_COUNT * 8:
        raise ValueError("truncated BSP header")
    if struct.unpack_from("<i", data, 0)[0] != 30:
        raise ValueError("unsupported BSP version")
    offset, length = struct.unpack_from("<ii", data, 4 + TEXTURE_LUMP * 8)
    if offset < 0 or length < 4 or offset + length > len(data):
        raise ValueError("invalid texture lump")
    count = struct.unpack_from("<i", data, offset)[0]
    if count < 0 or 4 + count * 4 > length:
        raise ValueError("invalid texture count")
    names: list[str] = []
    for index in range(count):
        relative = struct.unpack_from("<i", data, offset + 4 + index * 4)[0]
        if relative < 0 or relative + 16 > length:
            continue
        name = data[offset + relative:offset + relative + 16].split(b"\0", 1)[0].decode("latin-1").strip()
        if name:
            names.append(name)
    return names


def usable(name: str) -> bool:
    return not (name.startswith("{") or name.startswith("!") or name.casefold().startswith("sky"))


def source_index(root: Path) -> dict[str, Path]:
    indexed: dict[str, Path] = {}
    for wad in sorted(path for path in root.iterdir() if path.is_dir()):
        if wad.name.casefold() in EXCLUDED_WADS:
            continue
        for bitmap in wad.rglob("*.bmp"):
            key = bitmap.stem.casefold()
            if key in indexed and indexed[key].read_bytes() != bitmap.read_bytes():
                raise ValueError(f"ambiguous source texture {bitmap.stem}: {indexed[key]}, {bitmap}")
            indexed[key] = bitmap
    return indexed


def imagemagick() -> list[str]:
    if shutil.which("magick"):
        return ["magick"]
    if shutil.which("convert"):
        return ["convert"]
    raise RuntimeError("ImageMagick is required (install package imagemagick).")


def create_tile(source: Path, destination: Path, profile: str) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    command = imagemagick() + [
        str(source), "-colorspace", "Gray", "-resize", "128x128!",
        "(", "+clone", "-blur", "0x1.5", ")",
        "-compose", "Difference", "-composite", "-auto-level",
    ]
    if profile == "outline":
        command.extend(("-unsharp", "0x1.25+1.5+0.02", "-evaluate", "multiply", "0.55", "-level", "12%,88%"))
    else:
        command.extend(("-evaluate", "multiply", "0.35", "-level", "18%,82%"))
    command.append(str(destination))
    subprocess.run(command, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)


def build(maps: Path, textures: Path, overrides: Path, destination: Path, profile: str) -> tuple[int, int, int]:
    sources = source_index(textures)
    referenced: dict[str, set[str]] = defaultdict(set)
    spelling: dict[str, str] = {}
    for bsp in sorted(maps.glob("*.bsp")):
        try:
            names = bsp_texture_names(bsp)
        except ValueError as error:
            print(f"WARNING: skipped {bsp}: {error}")
            continue
        for name in names:
            key = name.casefold()
            if usable(name) and key in sources:
                referenced[bsp.stem].add(key)
                spelling.setdefault(key, name)

    detail_dir = destination / "gfx" / "detail"
    available = set(spelling)
    for key in sorted(available):
        create_tile(sources[key], detail_dir / f"{spelling[key]}.tga", profile)

    overridden = 0
    if overrides.is_dir():
        for override in sorted(overrides.glob("*.tga")):
            if override.stem.casefold() not in available:
                print(f"WARNING: ignored unused Steam detail override: {override}")
                continue
            shutil.copy2(override, detail_dir / override.name)
            overridden += 1

    maps_dir = destination / "maps"
    maps_dir.mkdir(parents=True, exist_ok=True)
    for map_name, keys in referenced.items():
        lines = [
            "// Generated GoldSrc detail overlays; base WAD/BSP textures remain unchanged.",
            "// TextureName detail/DetailTextureName scaleX scaleY",
        ]
        lines.extend(f"{spelling[key]} detail/{spelling[key]} 4.0 4.0" for key in sorted(keys))
        (maps_dir / f"{map_name}_detail.txt").write_text("\n".join(lines) + "\n", encoding="ascii")
    return len(available), overridden, len(referenced)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("maps", type=Path)
    parser.add_argument("textures", type=Path)
    parser.add_argument("overrides", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--profile", choices=("neutral", "outline"), default="outline")
    arguments = parser.parse_args()
    if not arguments.maps.is_dir() or not arguments.textures.is_dir():
        raise SystemExit("maps and texture source directories must exist")
    arguments.destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="gunman-steam-detail-", dir=arguments.destination.parent) as temporary_name:
        temporary = Path(temporary_name) / "output"
        generated, overridden, map_count = build(arguments.maps, arguments.textures, arguments.overrides, temporary, arguments.profile)
        if arguments.destination.exists():
            shutil.rmtree(arguments.destination)
        shutil.move(str(temporary), str(arguments.destination))
    print(f"Steam detail assets ({arguments.profile}): {generated} generated TGA, {overridden} overrides, {map_count} map detail files")


if __name__ == "__main__":
    main()
