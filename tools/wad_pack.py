#!/usr/bin/env python3
"""
WAD3-Texturpaket-Packer (GoldSrc) -- Gegenstueck zu `wad_extract.py`.

Liest alle 8-Bit-Palette-BMPs aus einem Ordner (Dateiname ohne .bmp wird zum
Textur-/Lump-Namen, max. 15 Zeichen + Nullterminierung wie im Original-Format)
und schreibt ein WAD3-Paket mit vollstaendiger Mip-Level-Kette (0/1/2/4/8 der
Originalgroesse, per Nearest-Neighbor-Downsampling erzeugt -- GoldSrc braucht
alle vier Level).

Voraussetzung: Breite und Hoehe jeder Textur muessen durch 8 teilbar sein
(Standard-GoldSrc-Einschraenkung fuer Miptex-Level 3). Texturen, die das nicht
erfuellen, werden uebersprungen und am Ende aufgelistet.

Nutzung:
    python3 wad_pack.py <input_ordner_mit_bmps> <output.wad>
"""
import struct
import sys
import os
import glob


def restore_goldsource_texture_name(filename):
    """Restore names sanitised by wad_extract.py for filesystem storage.

    GoldSrc uses leading '+' for animated frames and '!' for turbulent liquid
    textures.  The extractor maps non-filesystem characters to '_' so source
    trees remain portable.  These patterns are unambiguous in the extracted
    Gunman WAD sources and must be restored before the WAD directory is made.
    """
    if len(filename) >= 2 and filename[0] == "_" and filename[1] in "0123456789aA":
        return "+" + filename[1:]
    # The extractor replaces the leading '!' with '_'.  Some source trees
    # preserve the original upper case (`_W_BLUE1`), so normalise only for
    # matching while retaining the source spelling until the caller lowers it.
    lower_filename = filename.lower()
    if lower_filename.startswith("_swamp") or lower_filename.startswith("_w_") or lower_filename == "_labwatr":
        return "!" + filename[1:]
    return filename


def read_bmp_indexed(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[0:2] != b"BM":
        raise ValueError(f"{path}: keine BMP-Datei")
    (file_size, _, _, pixel_off) = struct.unpack_from("<IHHI", data, 2)
    (hdr_size, width, height, planes, bpp) = struct.unpack_from("<IiiHH", data, 14)
    if bpp != 8:
        raise ValueError(f"{path}: {bpp}-Bit-BMP nicht unterstuetzt (nur 8-Bit-Palette)")
    height_abs = abs(height)
    top_down = height < 0

    pal_off = 14 + hdr_size
    palette = []
    for i in range(256):
        b, g, r, _a = struct.unpack_from("<BBBB", data, pal_off + i * 4)
        palette.append((r, g, b))

    row_size = (width + 3) & ~3
    pixels = bytearray(width * height_abs)
    for y in range(height_abs):
        src_row = y if top_down else (height_abs - 1 - y)
        row_off = pixel_off + src_row * row_size
        pixels[y * width:(y + 1) * width] = data[row_off:row_off + width]
    return width, height_abs, bytes(pixels), palette


def downsample(pixels, width, height, factor, palette):
    """Nearest-Neighbor-Downsampling im Palette-Index-Raum (kein Farbmischen,
    um Palette-Treue zu wahren -- Standard-Vorgehen fuer Miptex-Level)."""
    nw, nh = width // factor, height // factor
    out = bytearray(nw * nh)
    for y in range(nh):
        sy = min(y * factor, height - 1)
        for x in range(nw):
            sx = min(x * factor, width - 1)
            out[y * nw + x] = pixels[sy * width + sx]
    return bytes(out)


def build_miptex(name, width, height, pixels, palette):
    mip0 = pixels
    mip1 = downsample(pixels, width, height, 2, palette)
    mip2 = downsample(pixels, width, height, 4, palette)
    mip3 = downsample(pixels, width, height, 8, palette)

    header_size = 16 + 4 + 4 + 16  # name + w + h + 4 offsets
    off0 = header_size
    off1 = off0 + len(mip0)
    off2 = off1 + len(mip1)
    off3 = off2 + len(mip2)

    name_bytes = name.encode("latin-1", errors="replace")[:15].ljust(16, b"\x00")
    buf = bytearray()
    buf += struct.pack("<16s I I I I I I", name_bytes, width, height, off0, off1, off2, off3)
    buf += mip0 + mip1 + mip2 + mip3
    buf += struct.pack("<h", 256)
    for (r, g, b) in palette:
        buf += struct.pack("<BBB", r, g, b)
    # 2 Byte Abschluss-Padding nach der Palette: gehoert zum WAD3-Miptex-Format
    # und ist in JEDEM Lump der Retail-WADs vorhanden (Vergleich Session 102:
    # alle unsere Lumps waren exakt 2 Byte kuerzer als die Retail-Gegenstuecke).
    buf += b"\x00\x00"
    return bytes(buf)


def build_qpic(width, height, pixels, palette):
    """Build a GoldSrc WAD3 QPic lump (type 0x42)."""
    buf = bytearray(struct.pack("<I I", width, height))
    buf += pixels
    buf += struct.pack("<h", 256)
    for (r, g, b) in palette:
        buf += struct.pack("<BBB", r, g, b)
    # Dasselbe 2-Byte-Padding wie beim Miptex - auch die QPic-Lumps der
    # Retail-WADs (cached/gfx) sind exakt 2 Byte laenger als unsere waren.
    buf += b"\x00\x00"
    return bytes(buf)


def pack_wad(in_dir, out_path, lump_kind="miptex"):
    bmp_files = sorted(glob.glob(os.path.join(in_dir, "*.bmp")))
    if not bmp_files:
        print(f"Keine .bmp-Dateien in {in_dir} gefunden.")
        return

    lumps = []
    skipped = []
    for path in bmp_files:
        # Texturnamen IMMER kleinschreiben: alle 1164 Namen der Retail-WADs sind
        # kleingeschrieben, waehrend unsere Quelldateien teils grossgeschriebene
        # Dateinamen haben (z.B. SKY.bmp). Das ist nicht kosmetisch - die
        # GoldSrc-Engine erkennt Spezialtexturen namensbasiert und dabei
        # case-sensitiv (Sky-Flaechen ueber strncmp(name,"sky",3)). Mit "SKY"
        # statt "sky" wurde die Skybox nicht als solche erkannt und stattdessen
        # die 16x16-Platzhaltertextur riesig gestreckt auf die Himmelsflaechen
        # gerendert (Nutzer-Bugreport Session 102, Screenshot CITY1A).
        name = restore_goldsource_texture_name(os.path.splitext(os.path.basename(path))[0]).lower()
        try:
            width, height, pixels, palette = read_bmp_indexed(path)
        except ValueError as e:
            skipped.append((name, str(e)))
            continue
        if lump_kind == "miptex" and (width % 8 != 0 or height % 8 != 0):
            skipped.append((name, f"Groesse {width}x{height} nicht durch 8 teilbar"))
            continue
        payload = build_miptex(name, width, height, pixels, palette) if lump_kind == "miptex" else build_qpic(width, height, pixels, palette)
        lumps.append((name, payload))

    header_size = 12
    dir_entry_size = 32
    body = bytearray()
    dir_entries = []
    lump_type = 0x43 if lump_kind == "miptex" else 0x42
    for name, payload in lumps:
        filepos = header_size + len(body)
        body += payload
        # 4-Byte-Ausrichtung zwischen Lumps (wie im Original ueblich)
        pad = (-len(body)) % 4
        body += b"\x00" * pad
        name_bytes = name.encode("latin-1", errors="replace")[:15].ljust(16, b"\x00")
        dir_entries.append(struct.pack(
            "<i i i b b h 16s", filepos, len(payload), len(payload), lump_type, 0, 0, name_bytes
        ))

    infotableofs = header_size + len(body)
    with open(out_path, "wb") as f:
        f.write(struct.pack("<4s i i", b"WAD3", len(lumps), infotableofs))
        f.write(bytes(body))
        for entry in dir_entries:
            f.write(entry)

    print(f"{out_path}: {len(lumps)} {lump_kind}-Lumps gepackt.")
    if skipped:
        print(f"  {len(skipped)} uebersprungen:")
        for name, reason in skipped:
            print(f"    - {name}: {reason}")


def main():
    arguments = sys.argv[1:]
    if len(arguments) not in (2, 3):
        print(f"Nutzung: {sys.argv[0]} <input_ordner_mit_bmps> <output.wad> [miptex|qpic]", file=sys.stderr)
        sys.exit(1)
    kind = arguments[2] if len(arguments) == 3 else "miptex"
    if kind not in ("miptex", "qpic"):
        print(f"Unbekannter Lump-Typ: {kind}", file=sys.stderr)
        sys.exit(1)
    pack_wad(arguments[0], arguments[1], kind)


if __name__ == "__main__":
    main()
