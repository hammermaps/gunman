#!/usr/bin/env python3
"""
WAD3-Texturpaket-Extraktor (GoldSrc) fuer die Gunman-Chronicles-RE-Doku.

Liest eine .wad-Datei (Half-Life/GoldSrc WAD3-Format, "Miptex"- und
"QPic"-Lumps mit eingebetteter 256-Farben-Palette) und schreibt jedes
unterstuetzte Bild als unkomprimiertes 8-Bit-Palette-BMP in einen Zielordner.

Format-Referenz (WAD3, unveraendert seit Half-Life 1, siehe z.B.
`re-project/src/halflife-updated-gm/utils/*`-Tools, die dasselbe Format
lesen/schreiben):

    wadheader_t:
        char magic[4]      "WAD3"
        int32 numlumps
        int32 infotableofs

    lumpinfo_t (an infotableofs, numlumps Eintraege a 32 Byte):
        int32 filepos
        int32 disksize
        int32 size          (unkomprimiert)
        char  type          (0x43 = Miptex)
        char  compression   (0 = keine)
        int16 pad
        char  name[16]

    miptex_t (an lumpinfo.filepos):
        char  name[16]
        uint32 width, height
        uint32 offsets[4]   (Mip-Level 0..3, relativ zum miptex_t-Start)
        <mip0 Byte-Indizes: width*height>
        <mip1: (width/2)*(height/2)>
        <mip2: (width/4)*(height/4)>
        <mip3: (width/8)*(height/8)>
        int16 numcolors     (immer 256)
        <Palette: 256 * 3 Byte RGB>

Texturnamen, die mit '{' beginnen, nutzen Palette-Index 255 als
Transparenzfarbe (klassisch Magenta) -- wird hier nur vermerkt, nicht
automatisch in eine Alpha-BMP umgewandelt (BMP kennt kein natives
Palette-Alpha; beim Weiterverarbeiten in einem Bildeditor beachten).

Nutzung:
    python3 wad_extract.py <input.wad> <output_ordner>
"""
import struct
import sys
import os


def write_bmp_indexed(path, width, height, pixels, palette):
    """Schreibt ein 8-Bit-Palette-BMP (Bottom-up, wie ueblich)."""
    row_size = (width + 3) & ~3  # 4-Byte-Zeilen-Alignment
    pixel_data_size = row_size * height
    palette_size = 256 * 4  # BGRA je Eintrag (Alpha ungenutzt)
    header_size = 14 + 40 + palette_size
    file_size = header_size + pixel_data_size

    with open(path, "wb") as f:
        # BITMAPFILEHEADER
        f.write(b"BM")
        f.write(struct.pack("<IHHI", file_size, 0, 0, header_size))
        # BITMAPINFOHEADER
        f.write(struct.pack(
            "<IiiHHIIiiII",
            40, width, height, 1, 8, 0, pixel_data_size, 2835, 2835, 256, 0
        ))
        # Palette (BGRA)
        for (r, g, b) in palette:
            f.write(struct.pack("<BBBB", b, g, r, 0))
        # Pixeldaten, Bottom-up
        for y in range(height - 1, -1, -1):
            row = pixels[y * width:(y + 1) * width]
            f.write(row)
            f.write(b"\x00" * (row_size - width))


def extract_qpic(data, filepos, size):
    """Liest einen GoldSrc-QPic-Lump (Typ 0x42).

    Das Format ist Breite/Hoehe, gefolgt von 8-Bit-Pixeln, einer 16-Bit-
    Farbanzahl und einer RGB-Palette. Es wird unter anderem fuer Console- und
    Loading-Bilder in cached.wad verwendet.
    """
    if size < 10:
        return None
    width, height = struct.unpack_from("<II", data, filepos)
    if width == 0 or height == 0 or width > 4096 or height > 4096:
        return None

    pixel_count = width * height
    pixels_start = filepos + 8
    colors_start = pixels_start + pixel_count
    if colors_start + 2 > filepos + size:
        return None
    color_count = struct.unpack_from("<H", data, colors_start)[0]
    palette_start = colors_start + 2
    if color_count != 256 or palette_start + color_count * 3 > filepos + size:
        return None

    pixels = data[pixels_start:colors_start]
    palette_bytes = data[palette_start:palette_start + color_count * 3]
    palette = [tuple(palette_bytes[j * 3:j * 3 + 3]) for j in range(color_count)]
    return width, height, pixels, palette


def extract_wad(wad_path, out_dir):
    with open(wad_path, "rb") as f:
        data = f.read()

    magic, numlumps, infotableofs = struct.unpack_from("<4s i i", data, 0)
    magic = magic.decode("latin-1", errors="replace")
    if magic != "WAD3":
        print(f"WARNUNG: Magic ist {magic!r}, erwartet 'WAD3' "
              f"(Datei evtl. kein GoldSrc-Texturpaket).")

    os.makedirs(out_dir, exist_ok=True)
    count = 0
    skipped = []

    for i in range(numlumps):
        loff = infotableofs + i * 32
        filepos, disksize, size, ltype, compression, pad, name_raw = struct.unpack_from(
            "<i i i b b h 16s", data, loff
        )
        name = name_raw.split(b"\x00", 1)[0].decode("latin-1", errors="replace")

        if ltype == 0x42:
            qpic = extract_qpic(data, filepos, disksize)
            if qpic is None:
                skipped.append((name, "ungueltiges oder nicht unterstuetztes QPic"))
                continue
            width, height, pixels, palette = qpic
            safe_name = "".join(c if c.isalnum() or c in "._-{}" else "_" for c in name)
            write_bmp_indexed(os.path.join(out_dir, safe_name + ".bmp"), width, height, pixels, palette)
            count += 1
            continue

        if ltype != 0x43:
            skipped.append((name, f"Lump-Typ 0x{ltype:02x} (kein Miptex, uebersprungen)"))
            continue
        if compression != 0:
            skipped.append((name, f"Kompression {compression} nicht unterstuetzt"))
            continue

        mname_raw, width, height, off0, off1, off2, off3 = struct.unpack_from(
            "<16s I I I I I I", data, filepos
        )
        if width == 0 or height == 0 or width > 4096 or height > 4096:
            skipped.append((name, f"unplausible Groesse {width}x{height}, uebersprungen"))
            continue

        mip0_off = filepos + off0
        mip0 = data[mip0_off:mip0_off + width * height]
        # Palette liegt nach dem letzten Mip-Level (Mip3) plus 2-Byte "numcolors"
        mip3_size = (width // 8) * (height // 8)
        pal_off = filepos + off3 + mip3_size + 2
        palette_bytes = data[pal_off:pal_off + 256 * 3]
        if len(palette_bytes) < 256 * 3 or len(mip0) < width * height:
            skipped.append((name, "Daten abgeschnitten/unvollstaendig, uebersprungen"))
            continue
        palette = [tuple(palette_bytes[j * 3:j * 3 + 3]) for j in range(256)]

        safe_name = "".join(c if c.isalnum() or c in "._-{}" else "_" for c in name)
        out_path = os.path.join(out_dir, safe_name + ".bmp")
        write_bmp_indexed(out_path, width, height, mip0, palette)
        count += 1

    print(f"{wad_path}: {count} Texturen -> {out_dir}")
    if skipped:
        print(f"  {len(skipped)} uebersprungen:")
        for name, reason in skipped:
            print(f"    - {name}: {reason}")


def main():
    if len(sys.argv) != 3:
        print(f"Nutzung: {sys.argv[0]} <input.wad> <output_ordner>", file=sys.stderr)
        sys.exit(1)
    extract_wad(sys.argv[1], sys.argv[2])


if __name__ == "__main__":
    main()
