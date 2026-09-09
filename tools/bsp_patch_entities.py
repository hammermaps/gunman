#!/usr/bin/env python3
"""Patcht den Entity-Lump (Lump 0) einer bereits kompilierten GoldSrc-.bsp,
indem zusaetzliche Entity-Bloecke angehaengt werden, und schreibt das
Ergebnis in eine NEUE Datei (Original bleibt unangetastet).

Kein BSP-Compiler noetig: der neue Entity-Text wird ans Dateiende
angehaengt, nur der Lump-0-Directory-Eintrag (Offset/Laenge) wird
aktualisiert - alle anderen Lumps (Geometrie/Vis/Lighting) behalten ihre
originalen Datei-Offsets und bleiben unveraendert. Nuetzlich, um neue
Point-Entities (z.B. rekonstruierte Cut-Content-Monster) ohne eigenen
qcsg/qbsp2/visx2/qrad-Toolchain-Build in eine bereits gebaute Karte
einzufuegen - siehe findings/entities/cutcontent_penta_batterybot.md
fuer ein Anwendungsbeispiel.

Format-Referenz: Header-Layout (int version; lump_t lumps[15]) wortwoertlich
aus src/halflife-updated-gm/utils/common/bspfile.h uebernommen.

Nutzung:
    python3 bsp_patch_entities.py <quelle.bsp> <ziel.bsp> <zusatz_entities.txt>

<zusatz_entities.txt> enthaelt einen oder mehrere GoldSrc-Entity-Bloecke im
Klartext-Format (wie von bsp_decompile.py erzeugt), z.B.:

    {
    "origin" "800 -88 -200"
    "classname" "monster_penta"
    }
"""
import struct
import sys

HEADER_LUMPS = 15


def main():
    if len(sys.argv) != 4:
        print(__doc__)
        sys.exit(1)

    src, dst, extra_ent_text = sys.argv[1], sys.argv[2], sys.argv[3]
    with open(src, "rb") as f:
        data = bytearray(f.read())

    version, = struct.unpack_from("<i", data, 0)
    assert version == 30, f"unerwartete BSPVERSION {version}"

    lumps = []
    for i in range(HEADER_LUMPS):
        ofs, length = struct.unpack_from("<ii", data, 4 + i * 8)
        lumps.append([ofs, length])

    ent_ofs, ent_len = lumps[0]
    ent_text = data[ent_ofs:ent_ofs + ent_len].decode("latin-1")

    assert ent_text.rstrip("\x00").rstrip().endswith("}"), "Entity-Lump sieht nicht wie gueltiger GoldSrc-Klartext aus"
    with open(extra_ent_text, "r", encoding="utf-8") as f:
        extra = f.read()

    trimmed = ent_text.rstrip("\x00")
    new_ent_text = trimmed.rstrip() + "\n" + extra.strip() + "\n\x00"
    new_ent_bytes = new_ent_text.encode("latin-1")

    new_ofs = len(data)
    data.extend(new_ent_bytes)
    lumps[0] = [new_ofs, len(new_ent_bytes)]

    for i in range(HEADER_LUMPS):
        struct.pack_into("<ii", data, 4 + i * 8, lumps[i][0], lumps[i][1])

    with open(dst, "wb") as f:
        f.write(data)

    print(f"OK: {dst} geschrieben ({len(data)} Bytes, neuer Entity-Lump {len(new_ent_bytes)} Bytes @ {new_ofs})")


if __name__ == "__main__":
    main()
