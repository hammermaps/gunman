#!/usr/bin/env python3
"""Read and write classic GoldSrc SPR v1/v2 sprites without third-party APIs.

The packer writes indexed SPR v2 single-frame entries from 8-bit BMP files.
Unpack exports all v1/v2 frames as indexed BMP plus a sprite.json manifest.
"""
import argparse
import json
import math
import struct
from pathlib import Path

IDENT = b"IDSP"
VERSION_1 = 1
VERSION_2 = 2
HEADER_V1 = struct.Struct("<4siifiiifi")
HEADER_V2 = struct.Struct("<4siiifiiifi")
FRAME = struct.Struct("<iiii")
MAX_DIMENSION = 256

TYPE_NAMES = ["vp_parallel_upright", "facing_upright", "vp_parallel", "oriented", "vp_parallel_oriented"]
FORMAT_NAMES = ["normal", "additive", "indexalpha", "alphatest"]


def read_bmp(path: Path):
    data = path.read_bytes()
    if data[:2] != b"BM" or len(data) < 54:
        raise ValueError(f"{path}: not a BMP")
    offset = struct.unpack_from("<I", data, 10)[0]
    header_size, width, height, planes, bpp = struct.unpack_from("<IiiHH", data, 14)
    if header_size != 40 or planes != 1 or bpp != 8 or width <= 0 or height == 0:
        raise ValueError(f"{path}: only uncompressed 8-bit BMPs are supported")
    compression = struct.unpack_from("<I", data, 30)[0]
    if compression != 0:
        raise ValueError(f"{path}: compressed BMPs are not supported")
    height_abs = abs(height)
    palette_offset = 14 + header_size
    if len(data) < palette_offset + 1024:
        raise ValueError(f"{path}: palette is incomplete")
    palette = [tuple(data[palette_offset + i * 4 + j] for j in (2, 1, 0)) for i in range(256)]
    stride = (width + 3) & ~3
    if len(data) < offset + stride * height_abs:
        raise ValueError(f"{path}: pixel data is incomplete")
    pixels = bytearray(width * height_abs)
    for y in range(height_abs):
        source_y = y if height < 0 else height_abs - 1 - y
        begin = offset + source_y * stride
        pixels[y * width:(y + 1) * width] = data[begin:begin + width]
    return width, height_abs, bytes(pixels), palette


def write_bmp(path: Path, width: int, height: int, pixels: bytes, palette):
    stride = (width + 3) & ~3
    pixel_offset = 14 + 40 + 1024
    file_size = pixel_offset + stride * height
    out = bytearray(struct.pack("<2sIHHI", b"BM", file_size, 0, 0, pixel_offset))
    out += struct.pack("<IiiHHIIiiII", 40, width, height, 1, 8, 0, stride * height, 0, 0, 256, 256)
    for red, green, blue in palette:
        out += bytes((blue, green, red, 0))
    for y in range(height - 1, -1, -1):
        out += pixels[y * width:(y + 1) * width]
        out += b"\0" * (stride - width)
    path.write_bytes(out)


def read_frame(data: bytes, offset: int):
    ox, oy, width, height = FRAME.unpack_from(data, offset)
    offset += FRAME.size
    if width <= 0 or height <= 0 or offset + width * height > len(data):
        raise ValueError("invalid or truncated sprite frame")
    pixels = data[offset:offset + width * height]
    return offset + width * height, {"origin": [ox, oy], "width": width, "height": height, "pixels": pixels}


def unpack_sprite(source: Path, destination: Path):
    data = source.read_bytes()
    if len(data) < HEADER_V1.size or data[:4] != IDENT:
        raise ValueError(f"{source}: not a GoldSrc SPR")
    version = struct.unpack_from("<i", data, 4)[0]
    if version == VERSION_1:
        _ident, _version, spr_type, radius, width, height, count, beam, sync = HEADER_V1.unpack_from(data)
        texture_format, offset = 0, HEADER_V1.size
        palette = [(i, i, i) for i in range(256)]
        palette_note = "SPR v1 has no embedded palette; grayscale indices were exported."
    elif version == VERSION_2:
        _ident, _version, spr_type, texture_format, radius, width, height, count, beam, sync = HEADER_V2.unpack_from(data)
        offset = HEADER_V2.size
        colors = struct.unpack_from("<H", data, offset)[0]
        offset += 2
        if colors != 256 or offset + colors * 3 > len(data):
            raise ValueError("SPR v2 palette is missing or not 256 colors")
        palette = [tuple(data[offset + i * 3:offset + i * 3 + 3]) for i in range(colors)]
        offset += colors * 3
        palette_note = None
    else:
        raise ValueError(f"unsupported SPR version {version}")

    destination.mkdir(parents=True, exist_ok=True)
    manifest = {"version": version, "type": spr_type, "texture_format": texture_format,
                "bounding_radius": radius, "width": width, "height": height,
                "beam_length": beam, "sync_type": sync, "frames": []}
    if palette_note:
        manifest["warning"] = palette_note
    for frame_index in range(count):
        frame_type = struct.unpack_from("<i", data, offset)[0]
        offset += 4
        if frame_type == 0:
            offset, frame = read_frame(data, offset)
            name = f"frame_{frame_index:03d}.bmp"
            write_bmp(destination / name, frame["width"], frame["height"], frame["pixels"], palette)
            manifest["frames"].append({"kind": "single", "file": name, "origin": frame["origin"]})
        elif frame_type == 1:
            group_count = struct.unpack_from("<i", data, offset)[0]
            offset += 4
            intervals = list(struct.unpack_from(f"<{group_count}f", data, offset))
            offset += group_count * 4
            group = {"kind": "group", "intervals": intervals, "frames": []}
            for group_index in range(group_count):
                offset, frame = read_frame(data, offset)
                name = f"frame_{frame_index:03d}_{group_index:03d}.bmp"
                write_bmp(destination / name, frame["width"], frame["height"], frame["pixels"], palette)
                group["frames"].append({"file": name, "origin": frame["origin"]})
            manifest["frames"].append(group)
        else:
            raise ValueError(f"unsupported frame type {frame_type}")
    (destination / "sprite.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"{source}: extracted {count} frame entry/entries to {destination}")


def pack_sprite(source: Path, destination: Path):
    manifest_path = source / "sprite.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8")) if manifest_path.exists() else {}
    frame_files = sorted(source.glob("frame_*.bmp"))
    if not frame_files:
        raise ValueError(f"{source}: no frame_*.bmp files")
    frames, palette = [], None
    for path in frame_files:
        width, height, pixels, current_palette = read_bmp(path)
        if width > MAX_DIMENSION or height > MAX_DIMENSION or width % 8 or height % 8:
            raise ValueError(f"{path}: dimensions must be <= {MAX_DIMENSION} and divisible by 8")
        if palette is None:
            palette = current_palette
        elif palette != current_palette:
            raise ValueError(f"{path}: palette differs from the first frame")
        frames.append((path.name, width, height, pixels))
    spr_type = int(manifest.get("type", 2))
    texture_format = int(manifest.get("texture_format", 1))
    origins = {entry.get("file"): entry.get("origin") for entry in manifest.get("frames", []) if entry.get("kind") == "single"}
    max_width, max_height = max(item[1] for item in frames), max(item[2] for item in frames)
    radius = math.hypot(max_width // 2, max_height // 2)
    out = bytearray(HEADER_V2.pack(IDENT, VERSION_2, spr_type, texture_format, radius,
                                   max_width, max_height, len(frames), float(manifest.get("beam_length", 0)),
                                   int(manifest.get("sync_type", 0))))
    out += struct.pack("<H", 256)
    out += bytes(component for color in palette for component in color)
    for name, width, height, pixels in frames:
        origin = origins.get(name, [-(width // 2), height // 2])
        out += struct.pack("<i", 0)
        out += FRAME.pack(int(origin[0]), int(origin[1]), width, height)
        out += pixels
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(out)
    print(f"{destination}: packed {len(frames)} SPR v2 frame(s)")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    unpack = commands.add_parser("unpack", help="extract SPR frames and sprite.json")
    unpack.add_argument("source", type=Path)
    unpack.add_argument("destination", type=Path)
    pack = commands.add_parser("pack", help="pack indexed BMP frames into SPR v2")
    pack.add_argument("source", type=Path)
    pack.add_argument("destination", type=Path)
    args = parser.parse_args()
    if args.command == "unpack":
        unpack_sprite(args.source, args.destination)
    else:
        pack_sprite(args.source, args.destination)


if __name__ == "__main__":
    main()
