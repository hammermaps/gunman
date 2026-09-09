#!/usr/bin/env python3
"""Build Xash3D external TGA texture replacements from the WAD BMP sources.

Generated BMPs live in textures-src/base/<wad-name>.  An artist can place a
same-named TGA (typically a high-resolution replacement) in
textures-src/xash-overrides/<wad-name>; it then takes precedence over the
generated TGA during staging.
"""
from __future__ import annotations

import argparse
import shutil
import struct
from pathlib import Path


def read_indexed_bmp(path: Path) -> tuple[int, int, bytes, list[tuple[int, int, int]]]:
    data = path.read_bytes()
    if len(data) < 54 or data[:2] != b"BM":
        raise ValueError("not a BMP file")
    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    dib_size, width, height, planes, bits, compression = struct.unpack_from("<IiiHHI", data, 14)
    if dib_size < 40 or width <= 0 or height == 0 or planes != 1 or bits != 8 or compression != 0:
        raise ValueError("expected uncompressed 8-bit BMP")
    top_down = height < 0
    height = abs(height)
    palette_offset = 14 + dib_size
    if pixel_offset < palette_offset + 256 * 4:
        raise ValueError("missing 256-colour palette")
    palette = []
    for index in range(256):
        blue, green, red, _ = struct.unpack_from("<BBBB", data, palette_offset + index * 4)
        palette.append((red, green, blue))

    row_size = (width + 3) & ~3
    if pixel_offset + row_size * height > len(data):
        raise ValueError("truncated pixel data")
    rows = [data[pixel_offset + row * row_size:pixel_offset + row * row_size + width] for row in range(height)]
    if not top_down:
        rows.reverse()
    return width, height, b"".join(rows), palette


def write_tga(source: Path, destination: Path) -> None:
    width, height, pixels, palette = read_indexed_bmp(source)
    transparent_index = 255 if source.stem.startswith("{") else None
    destination.parent.mkdir(parents=True, exist_ok=True)
    with destination.open("wb") as output:
        # Uncompressed BGRA true-colour TGA, top-left origin, 8-bit alpha.
        output.write(struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, width, height, 32, 0x28))
        for palette_index in pixels:
            red, green, blue = palette[palette_index]
            alpha = 0 if palette_index == transparent_index else 255
            output.write(bytes((blue, green, red, alpha)))


def material_group(wad_name: str) -> str:
    return "decals" if wad_name.casefold() == "decals" else "common"


def stage(source_root: Path, override_root: Path, destination_root: Path, hd_root: Path | None) -> tuple[int, int, int]:
    generated = 0
    hd_generated = 0
    overridden = 0
    wad_dirs = sorted(path for path in source_root.iterdir() if path.is_dir())
    if not wad_dirs:
        raise ValueError(f"no extracted WAD directories in {source_root}")

    for wad_dir in wad_dirs:
        target_dir = destination_root / material_group(wad_dir.name)
        override_dir = override_root / wad_dir.name
        for bitmap in sorted(wad_dir.rglob("*.bmp")):
            relative = bitmap.relative_to(wad_dir).with_suffix(".tga")
            target = target_dir / relative
            override = override_dir / relative
            hd_texture = hd_root / wad_dir.name / relative if hd_root is not None else None
            if override.is_file():
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(override, target)
                overridden += 1
            elif hd_texture is not None and hd_texture.is_file():
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(hd_texture, target)
                hd_generated += 1
            else:
                write_tga(bitmap, target)
                generated += 1

        # Allow a wholly new replacement only when it corresponds to an
        # extracted texture name. This avoids silently staging mistyped files.
        if override_dir.is_dir():
            for override in sorted(override_dir.rglob("*.tga")):
                expected = wad_dir / override.relative_to(override_dir).with_suffix(".bmp")
                if not expected.is_file():
                    print(f"WARNING: ignored override without source texture: {override}")

    return generated, hd_generated, overridden


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="textures-src/base")
    parser.add_argument("overrides", type=Path, help="textures-src/xash-overrides")
    parser.add_argument("destination", type=Path, help="rewolf/materials")
    parser.add_argument("--hd-source", type=Path, help="optional generated HD TGA root")
    arguments = parser.parse_args()
    generated, hd_generated, overridden = stage(arguments.source, arguments.overrides, arguments.destination, arguments.hd_source)
    print(f"Xash3D materials staged: {hd_generated} HD TGA, {generated} fallback TGA, {overridden} override TGA")


if __name__ == "__main__":
    main()
