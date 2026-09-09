#!/usr/bin/env python3
"""
GoldSrc-BSP-Decompiler (Version 30) fuer die Gunman-Chronicles-RE-Doku.

Zerlegt eine .bsp-Datei in editierbare/inspizierbare Quelldateien:

    <mapname>.ent            Entity-Lump als Klartext (Keyvalues, wie im
                              Original -- dasselbe, das bisher per
                              "grep -a"/Byte-Offset-Extraktion aus BSPs
                              gezogen wurde, jetzt aber vollstaendig und
                              strukturiert je Map als eigene Datei)
    textures/*.bmp            eingebettete WAD-Texturen (falls die Map ihre
                              Texturen nicht rein aus einer externen .wad
                              bezieht -- siehe worldspawn-Keyvalue "wad")
    textures/EXTERNAL.txt     Liste der Texturen OHNE eingebettete Pixeldaten
                              (nur Name/Groesse bekannt) -- diese liegen in
                              einer externen .wad, siehe
                              re-project/textures-src/
    <mapname>.obj + .mtl      trianguliertes Referenz-Netz der GESAMTEN
                              sichtbaren Weltgeometrie (alle Faces aller
                              BSP-Modelle, inkl. func_-Brush-Entities),
                              nach Textur gruppiert, mit UV-Koordinaten

WICHTIG -- was dieses Tool NICHT tut:
Es rekonstruiert **keine editierbaren Brushes/.map-Datei**. Aus dem
kompilierten BSP-Baum (Planes/Nodes/Clipnodes) wieder echte, editierbare
konvexe Brushes zu gewinnen ("BSP-zu-MAP", wie es z.B. J.A.C.K./Hammer als
Import-Funktion oder dedizierte Tools wie "Bsp2Map" tun) ist ein eigenes,
deutlich komplexeres und fehleranfaelliges Verfahren (konvexe-Huellen-
Rekonstruktion aus Clipnodes) und war in dieser Session nicht das Ziel. Das
hier erzeugte OBJ ist ein reines **Sichtpruef-/Referenz-Mesh** (Dreiecke,
keine Brushes) -- gut zum Betrachten/Vergleichen der Level-Geometrie in
Blender o.ae., aber nicht direkt in Hammer/J.A.C.K. weiterbearbeitbar.

Format-Referenz: alle Struct-Layouts sind wortwoertlich aus
`re-project/src/halflife-updated-gm/utils/common/bspfile.h` uebernommen
(demselben Engine-Fork, den Gunman Chronicles nutzt).

Nutzung:
    python3 bsp_decompile.py <karte.bsp> <ausgabe_ordner>
"""
import struct
import sys
import os
import argparse


LUMP_ENTITIES = 0
LUMP_PLANES = 1
LUMP_TEXTURES = 2
LUMP_VERTEXES = 3
LUMP_VISIBILITY = 4
LUMP_NODES = 5
LUMP_TEXINFO = 6
LUMP_FACES = 7
LUMP_LIGHTING = 8
LUMP_CLIPNODES = 9
LUMP_LEAFS = 10
LUMP_MARKSURFACES = 11
LUMP_EDGES = 12
LUMP_SURFEDGES = 13
LUMP_MODELS = 14
HEADER_LUMPS = 15


def write_bmp_indexed(path, width, height, pixels, palette):
    row_size = (width + 3) & ~3
    pixel_data_size = row_size * height
    palette_size = 256 * 4
    header_size = 14 + 40 + palette_size
    file_size = header_size + pixel_data_size
    with open(path, "wb") as f:
        f.write(b"BM")
        f.write(struct.pack("<IHHI", file_size, 0, 0, header_size))
        f.write(struct.pack("<IiiHHIIiiII", 40, width, height, 1, 8, 0,
                             pixel_data_size, 2835, 2835, 256, 0))
        for (r, g, b) in palette:
            f.write(struct.pack("<BBBB", b, g, r, 0))
        for y in range(height - 1, -1, -1):
            row = pixels[y * width:(y + 1) * width]
            f.write(row)
            f.write(b"\x00" * (row_size - width))


class Bsp:
    def __init__(self, path):
        with open(path, "rb") as f:
            self.data = f.read()
        (self.version,) = struct.unpack_from("<i", self.data, 0)
        self.lumps = []
        for i in range(HEADER_LUMPS):
            ofs, length = struct.unpack_from("<ii", self.data, 4 + i * 8)
            self.lumps.append((ofs, length))

    def lump(self, idx):
        ofs, length = self.lumps[idx]
        return self.data[ofs:ofs + length]

    def entities_text(self):
        raw = self.lump(LUMP_ENTITIES)
        return raw.split(b"\x00", 1)[0].decode("latin-1", errors="replace")

    def textures(self):
        """Liest die dmiptexlump_t + je Eintrag ein miptex_t. Liefert Liste von
        dicts: name, width, height, embedded(bool), pixels/palette (falls
        eingebettet)."""
        lump_ofs, lump_len = self.lumps[LUMP_TEXTURES]
        d = self.data
        (nummiptex,) = struct.unpack_from("<i", d, lump_ofs)
        offsets = struct.unpack_from(f"<{nummiptex}i", d, lump_ofs + 4)
        out = []
        for ofs in offsets:
            if ofs == -1:
                out.append(None)
                continue
            moff = lump_ofs + ofs
            name_raw, width, height, o0, o1, o2, o3 = struct.unpack_from(
                "<16s I I I I I I", d, moff
            )
            name = name_raw.split(b"\x00", 1)[0].decode("latin-1", errors="replace")
            entry = {"name": name, "width": width, "height": height, "embedded": False}
            if o0 != 0:
                mip0 = d[moff + o0:moff + o0 + width * height]
                mip3_size = (width // 8) * (height // 8)
                pal_off = moff + o3 + mip3_size + 2
                palbytes = d[pal_off:pal_off + 256 * 3]
                if len(mip0) == width * height and len(palbytes) == 256 * 3:
                    entry["embedded"] = True
                    entry["pixels"] = mip0
                    entry["palette"] = [tuple(palbytes[j * 3:j * 3 + 3]) for j in range(256)]
            out.append(entry)
        return out

    def vertexes(self):
        raw = self.lump(LUMP_VERTEXES)
        n = len(raw) // 12
        return list(struct.iter_unpack("<3f", raw[:n * 12]))

    def edges(self):
        raw = self.lump(LUMP_EDGES)
        n = len(raw) // 4
        return list(struct.iter_unpack("<HH", raw[:n * 4]))

    def surfedges(self):
        raw = self.lump(LUMP_SURFEDGES)
        n = len(raw) // 4
        return list(v[0] for v in struct.iter_unpack("<i", raw[:n * 4]))

    def texinfos(self):
        raw = self.lump(LUMP_TEXINFO)
        n = len(raw) // 40  # 2*4*4 + 4 + 4 = 40 Byte je texinfo_t
        out = []
        for i in range(n):
            vals = struct.unpack_from("<8f i i", raw, i * 40)
            vecs = (vals[0:4], vals[4:8])
            miptex, flags = vals[8], vals[9]
            out.append({"vecs": vecs, "miptex": miptex, "flags": flags})
        return out

    def faces(self):
        raw = self.lump(LUMP_FACES)
        n = len(raw) // 20  # dface_t = 20 Byte
        out = []
        for i in range(n):
            planenum, side, firstedge, numedges, texinfo = struct.unpack_from(
                "<hh i h h", raw, i * 20
            )
            out.append({"firstedge": firstedge, "numedges": numedges, "texinfo": texinfo})
        return out


def decompile(bsp_path, out_dir):
    bsp = Bsp(bsp_path)
    if bsp.version != 30:
        print(f"WARNUNG: BSP-Version {bsp.version}, erwartet 30 (GoldSrc). "
              f"Struct-Layouts koennten nicht passen.")

    base = os.path.splitext(os.path.basename(bsp_path))[0]
    os.makedirs(out_dir, exist_ok=True)

    # --- Entities ---
    ent_text = bsp.entities_text()
    ent_path = os.path.join(out_dir, base + ".ent")
    with open(ent_path, "w", encoding="utf-8") as f:
        f.write(ent_text)
    ent_count = ent_text.count("classname")
    print(f"  Entities: {ent_path} (~{ent_count} Entities)")

    # --- Texturen ---
    tex_dir = os.path.join(out_dir, "textures")
    os.makedirs(tex_dir, exist_ok=True)
    textures = bsp.textures()
    embedded_count = 0
    external = []
    for tex in textures:
        if tex is None:
            continue
        if tex["embedded"]:
            safe_name = "".join(c if c.isalnum() or c in "._-{}" else "_" for c in tex["name"])
            write_bmp_indexed(os.path.join(tex_dir, safe_name + ".bmp"),
                               tex["width"], tex["height"], tex["pixels"], tex["palette"])
            embedded_count += 1
        else:
            external.append(tex)
    if external:
        with open(os.path.join(tex_dir, "EXTERNAL.txt"), "w", encoding="utf-8") as f:
            f.write("# Texturen ohne eingebettete Pixeldaten (aus externer .wad,\n")
            f.write("# siehe re-project/textures-src/rewolf/<variante>/<name>.bmp):\n")
            for tex in external:
                f.write(f"{tex['name']}\t{tex['width']}x{tex['height']}\n")
    print(f"  Texturen: {embedded_count} eingebettet, {len(external)} extern "
          f"(siehe textures/EXTERNAL.txt)")

    # --- Referenz-Mesh (OBJ) ---
    verts = bsp.vertexes()
    edges = bsp.edges()
    surfedges = bsp.surfedges()
    texinfos = bsp.texinfos()
    faces = bsp.faces()

    obj_path = os.path.join(out_dir, base + ".obj")
    mtl_path = os.path.join(out_dir, base + ".mtl")

    tex_by_group = {}
    for face in faces:
        ti = texinfos[face["texinfo"]] if 0 <= face["texinfo"] < len(texinfos) else None
        miptex_i = int(ti["miptex"]) if ti else -1
        tex = textures[miptex_i] if ti and 0 <= miptex_i < len(textures) and textures[miptex_i] else None
        tex_name = tex["name"] if tex else "notexture"
        tw = tex["width"] if tex else 1
        th = tex["height"] if tex else 1

        loop = []
        for k in range(face["numedges"]):
            se = surfedges[face["firstedge"] + k]
            if se >= 0:
                vidx = edges[se][0]
            else:
                vidx = edges[-se][1]
            loop.append(vidx)
        if len(loop) < 3:
            continue

        group = tex_by_group.setdefault(tex_name, {"tw": tw, "th": th, "faces": []})
        group["faces"].append((loop, ti))

    with open(mtl_path, "w", encoding="utf-8") as f:
        for tex_name in tex_by_group:
            f.write(f"newmtl {tex_name}\n")
            f.write(f"Kd 1.0 1.0 1.0\n")
            f.write(f"map_Kd textures/{tex_name}.bmp\n\n")

    with open(obj_path, "w", encoding="utf-8") as f:
        f.write(f"mtllib {base}.mtl\n")
        f.write(f"o {base}\n")
        for (x, y, z) in verts:
            f.write(f"v {x:.4f} {y:.4f} {z:.4f}\n")

        vt_index = 1
        for tex_name, group in tex_by_group.items():
            f.write(f"usemtl {tex_name}\n")
            tw, th = group["tw"], group["th"]
            for (loop, ti) in group["faces"]:
                if ti:
                    (sx, sy, sz, so) = ti["vecs"][0]
                    (tx, ty, tz, to) = ti["vecs"][1]
                else:
                    (sx, sy, sz, so) = (1, 0, 0, 0)
                    (tx, ty, tz, to) = (0, 1, 0, 0)
                uvs = []
                for vidx in loop:
                    x, y, z = verts[vidx]
                    u = (x * sx + y * sy + z * sz + so) / tw
                    v = (x * tx + y * ty + z * tz + to) / th
                    f.write(f"vt {u:.5f} {-v:.5f}\n")
                    uvs.append(vt_index)
                    vt_index += 1
                # Faechertriangulation (Faces sind konvexe, planare Polygone)
                for i in range(1, len(loop) - 1):
                    a, b, c = loop[0], loop[i], loop[i + 1]
                    ua, ub, uc = uvs[0], uvs[i], uvs[i + 1]
                    f.write(f"f {a + 1}/{ua} {b + 1}/{ub} {c + 1}/{uc}\n")

    print(f"  Referenz-Mesh: {obj_path} ({len(verts)} Vertices, {len(faces)} Faces, "
          f"{len(tex_by_group)} Texturgruppen)")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("bsp", help="Pfad zur .bsp-Datei")
    ap.add_argument("out_dir", help="Zielordner fuer die dekompilierten Quelldateien")
    args = ap.parse_args()

    if not os.path.isfile(args.bsp):
        print(f"Datei nicht gefunden: {args.bsp}", file=sys.stderr)
        sys.exit(1)

    print(f"=== {args.bsp} -> {args.out_dir} ===")
    decompile(args.bsp, args.out_dir)


if __name__ == "__main__":
    main()
