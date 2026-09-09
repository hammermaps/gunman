#!/usr/bin/env python3
"""Create reviewable 2x Xash3D texture candidates from extracted WAD BMPs."""
from __future__ import annotations

import argparse
import os
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path

from stage_xash_materials import read_indexed_bmp


def write_tga(path: Path, width: int, height: int, pixels: bytes, palette: list[tuple[int, int, int]], transparent: bool) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as output:
        output.write(struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, width, height, 32, 0x28))
        for index in pixels:
            red, green, blue = palette[index]
            alpha = 0 if transparent and index == 255 else 255
            output.write(bytes((blue, green, red, alpha)))


def normalized_tga(source: Path, target: Path) -> None:
    width, height, pixels, palette = read_indexed_bmp(source)
    write_tga(target, width, height, pixels, palette, source.stem.startswith("{"))


def nearest_2x(source: Path, target: Path) -> None:
    width, height, pixels, palette = read_indexed_bmp(source)
    scaled = bytearray()
    for row in range(height):
        expanded_row = b"".join(bytes((pixel, pixel)) for pixel in pixels[row * width:(row + 1) * width])
        scaled.extend(expanded_row)
        scaled.extend(expanded_row)
    write_tga(target, width * 2, height * 2, bytes(scaled), palette, source.stem.startswith("{"))


def imagemagick_command() -> list[str]:
    if shutil.which("magick"):
        return ["magick"]
    if shutil.which("convert"):
        return ["convert"]
    raise RuntimeError("ImageMagick is required for lanczos-2x (install package 'imagemagick').")


def lanczos_2x(source: Path, target: Path, temporary: Path) -> None:
    normalized = temporary / "input.tga"
    normalized_tga(source, normalized)
    target.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(imagemagick_command() + [str(normalized), "-filter", "Lanczos", "-resize", "200%", str(target)], check=True)


def ai_2x(source: Path, target: Path, temporary: Path) -> None:
    binary = os.environ.get("GUNMAN_AI_UPSCALER")
    if not binary:
        raise RuntimeError("ai-2x requires GUNMAN_AI_UPSCALER=/path/to/waifu2x-ncnn-vulkan.")
    if not Path(binary).is_file() and not shutil.which(binary):
        raise RuntimeError(f"AI upscaler not found: {binary}")
    input_png = temporary / "input.png"
    output_png = temporary / "output.png"
    normalized_tga(source, temporary / "input.tga")
    target.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(imagemagick_command() + [str(temporary / "input.tga"), str(input_png)], check=True)
    # waifu2x-ncnn-vulkan is portable on Linux/Windows/macOS and accepts this
    # argument set. Keep denoising at zero to avoid erasing original detail.
    command = [binary, "-i", str(input_png), "-o", str(output_png), "-n", "0", "-s", "2", "-f", "png"]
    gpu = os.environ.get("GUNMAN_AI_GPU")
    if gpu:
        command.extend(("-g", gpu))
    subprocess.run(command, check=True)
    subprocess.run(imagemagick_command() + [str(output_png), str(target)], check=True)


def selected_wads(source_root: Path, requested: str) -> list[Path]:
    if requested == "all":
        return sorted(path for path in source_root.iterdir() if path.is_dir())
    selected = source_root / requested
    if not selected.is_dir():
        raise RuntimeError(f"unknown WAD source directory: {selected}")
    return [selected]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("profile", choices=("nearest-2x", "lanczos-2x", "ai-2x"))
    parser.add_argument("wad", help="WAD directory name below textures-src/base, or 'all'")
    parser.add_argument("--publish", action="store_true", help="copy the generated TGAs into xash-overrides")
    parser.add_argument("--source", type=Path, default=Path("textures-src/base"))
    parser.add_argument("--preview-root", type=Path, default=Path("textures-src/xash-previews"))
    parser.add_argument("--override-root", type=Path, default=Path("textures-src/xash-overrides"))
    arguments = parser.parse_args()

    if not arguments.source.is_dir():
        raise SystemExit(f"missing extracted source: {arguments.source}")

    generated = 0
    for wad_dir in selected_wads(arguments.source, arguments.wad):
        destination_dir = arguments.preview_root / arguments.profile / wad_dir.name
        for source in sorted(wad_dir.rglob("*.bmp")):
            relative = source.relative_to(wad_dir).with_suffix(".tga")
            destination = destination_dir / relative
            with tempfile.TemporaryDirectory(prefix="gunman-upscale-") as temporary_name:
                temporary = Path(temporary_name)
                if arguments.profile == "nearest-2x":
                    nearest_2x(source, destination)
                elif arguments.profile == "lanczos-2x":
                    lanczos_2x(source, destination, temporary)
                else:
                    ai_2x(source, destination, temporary)
            generated += 1

        if arguments.publish:
            override_dir = arguments.override_root / wad_dir.name
            override_dir.mkdir(parents=True, exist_ok=True)
            for texture in destination_dir.rglob("*.tga"):
                shutil.copy2(texture, override_dir / texture.relative_to(destination_dir))

    action = "generated and published" if arguments.publish else "generated for review"
    print(f"{generated} TGA file(s) {action}: {arguments.preview_root / arguments.profile}")


if __name__ == "__main__":
    main()
