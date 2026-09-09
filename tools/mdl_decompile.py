#!/usr/bin/env python3
"""
GoldSrc-Studiomodell-Decompiler (.mdl, Version 10) fuer die
Gunman-Chronicles-RE-Doku.

Zerlegt eine .mdl-Datei in editierbare Quelldateien (Textur-BMPs,
Referenz-SMD mit Bind-Pose-Geometrie, ein SMD je Animationssequenz, sowie
ein QC-Kompilierskript) -- analog zu Tools wie Crowbar/HLMV, aber als reines
Python-Skript ohne Wine/externe Abhaengigkeiten.

WICHTIG -- Herkunft und Verifikationsstand der verwendeten Formeln:
Alle Struct-Layouts UND alle Transform-Formeln (AngleQuaternion,
QuaternionMatrix, R_ConcatTransforms, VectorTransform, die
Animationswert-Dekodierung aus CalcBonePosition/CalcBoneQuaternion sowie das
Dreiecks-Befehlslisten-Format) sind wortwoertlich aus dem tatsaechlichen
Gunman-Engine-Fork uebernommen:
  - re-project/src/halflife-updated-gm/engine/studio.h
  - re-project/src/halflife-updated-gm/utils/common/mathlib.cpp
  - re-project/src/halflife-updated-gm/utils/mdlviewer/studio_render.cpp
Keine geschaetzten/generischen GoldSrc-Annahmen. Die Textur- und
Bone-/Attachment-/Sequenz-Metadaten-Extraktion ist dieselbe, bereits
gegen echte Dateien gepruefte Logik wie in `mdl_inspect.py`/`wad_extract.py`.

**Verifikationsstand:** Die Referenz-Netz-Geometrie (Weltraum-Vertex-
Transformation ueber die Bone-Hierarchie) wurde in dieser Session NICHT in
einem 3D-Viewer/Editor sichtgeprueft (keine GUI/OpenGL in dieser Umgebung
verfuegbar) -- die Formeln sind 1:1 aus dem Referenz-Quellcode uebernommen,
aber ein Stichprobentest (SMD in Blender mit "Blender Source Tools" oder
per erneuter Kompilation via `studiomdl` laden und mit dem Original
vergleichen) wird empfohlen, bevor die exportierte Geometrie fuer
produktive Zwecke (Reimplementierung) als endgueltig verifiziert gilt.
Bone-Hierarchie/Attachments/Sequenz-Events/Texturen sind hingegen bereits
durch `mdl_inspect.py` gegen mehrere echte Modelle geprueft und zuverlaessig.

Nutzung:
    python3 mdl_decompile.py <modell.mdl> <ausgabe_ordner> [--no-anims]

Erzeugt in <ausgabe_ordner>:
    textures/*.bmp
    <basename>_ref.smd
    <basename>_seq_NN_<label>.smd   (eine Datei je Animationssequenz)
    <basename>.qc
"""
import struct
import sys
import os
import math
import argparse


def cstr(b):
    return b.split(b"\x00", 1)[0].decode("latin-1", errors="replace")


# ---------------------------------------------------------------------------
# Mathe-Grundlagen -- 1:1 aus utils/common/mathlib.cpp uebernommen.
# ---------------------------------------------------------------------------

def angle_quaternion(roll, pitch, yaw):
    """AngleQuaternion() aus mathlib.cpp. Eingaben in Radiant."""
    angle = yaw * 0.5
    sy, cy = math.sin(angle), math.cos(angle)
    angle = pitch * 0.5
    sp, cp = math.sin(angle), math.cos(angle)
    angle = roll * 0.5
    sr, cr = math.sin(angle), math.cos(angle)

    x = sr * cp * cy - cr * sp * sy
    y = cr * sp * cy + sr * cp * sy
    z = cr * cp * sy - sr * sp * cy
    w = cr * cp * cy + sr * sp * sy
    return (x, y, z, w)


def quaternion_matrix(q):
    """QuaternionMatrix() aus mathlib.cpp -> 3x3-Rotationsteil einer 3x4-Matrix."""
    x, y, z, w = q
    m = [[0.0] * 3 for _ in range(3)]
    m[0][0] = 1.0 - 2.0 * y * y - 2.0 * z * z
    m[1][0] = 2.0 * x * y + 2.0 * w * z
    m[2][0] = 2.0 * x * z - 2.0 * w * y

    m[0][1] = 2.0 * x * y - 2.0 * w * z
    m[1][1] = 1.0 - 2.0 * x * x - 2.0 * z * z
    m[2][1] = 2.0 * y * z + 2.0 * w * x

    m[0][2] = 2.0 * x * z + 2.0 * w * y
    m[1][2] = 2.0 * y * z - 2.0 * w * x
    m[2][2] = 1.0 - 2.0 * x * x - 2.0 * y * y
    return m


def make_bone_matrix(pos, quat):
    """3x4-Matrix [R|T] aus Position + Quaternion (wie SetUpBones())."""
    m3 = quaternion_matrix(quat)
    return [
        [m3[0][0], m3[0][1], m3[0][2], pos[0]],
        [m3[1][0], m3[1][1], m3[1][2], pos[1]],
        [m3[2][0], m3[2][1], m3[2][2], pos[2]],
    ]


def concat_transforms(a, b):
    """R_ConcatTransforms() aus mathlib.cpp -- out = a * b (3x4 Matrizen)."""
    out = [[0.0] * 4 for _ in range(3)]
    for r in range(3):
        for c in range(3):
            out[r][c] = a[r][0] * b[0][c] + a[r][1] * b[1][c] + a[r][2] * b[2][c]
        out[r][3] = a[r][0] * b[0][3] + a[r][1] * b[1][3] + a[r][2] * b[2][3] + a[r][3]
    return out


def vector_transform(v, m):
    """VectorTransform() aus mathlib.cpp."""
    return (
        v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m[0][2] + m[0][3],
        v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m[1][2] + m[1][3],
        v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m[2][2] + m[2][3],
    )


def vector_rotate(v, m):
    """Wie VectorTransform, aber ohne Translation (fuer Normalen)."""
    return (
        v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m[0][2],
        v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m[1][2],
        v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m[2][2],
    )


# ---------------------------------------------------------------------------
# BMP-Writer (identisch zu wad_extract.py)
# ---------------------------------------------------------------------------

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


# ---------------------------------------------------------------------------
# MDL-Parser
# ---------------------------------------------------------------------------

BONE_FMT = "<32s i i 6i 6f 6f"
BONE_SIZE = struct.calcsize(BONE_FMT)
ATT_FMT = "<32s i i 3f 9f"
ATT_SIZE = struct.calcsize(ATT_FMT)
BODYPART_FMT = "<64s i i i"
BODYPART_SIZE = struct.calcsize(BODYPART_FMT)
SEQ_FMT = "<32s f i i i i i i i i i i 3f i i 3f 3f i i 2i 2f 2f i i i i i i"
SEQ_SIZE = struct.calcsize(SEQ_FMT)
EVENT_FMT = "<i i i 64s"
EVENT_SIZE = struct.calcsize(EVENT_FMT)
TEXTURE_FMT = "<64s i i i i"
TEXTURE_SIZE = struct.calcsize(TEXTURE_FMT)
MODEL_FMT = "<64s i f 10i"
MODEL_SIZE = struct.calcsize(MODEL_FMT)
MESH_FMT = "<i i i i i"
MESH_SIZE = struct.calcsize(MESH_FMT)
ANIM_FMT = "<6H"
ANIM_SIZE = struct.calcsize(ANIM_FMT)
ANIMVALUE_SIZE = 2  # ein mstudioanimvalue_t ist immer 2 Byte (union short/2x byte)


class Mdl:
    def __init__(self, path):
        with open(path, "rb") as f:
            self.data = f.read()
        self.path = path
        self._parse_header()

    def _parse_header(self):
        d = self.data
        self.id, self.version = struct.unpack_from("<4s i", d, 0)
        self.id = self.id.decode("latin-1", errors="replace")
        name_raw, self.length = struct.unpack_from("<64s i", d, 8)
        self.name = cstr(name_raw)
        off = 8 + 64 + 4
        (self.eyeposition, self.min, self.max, self.bbmin, self.bbmax) = [
            struct.unpack_from("<3f", d, off + 12 * i) for i in range(5)
        ]
        off += 12 * 5
        (self.flags,) = struct.unpack_from("<i", d, off)
        off += 4
        (
            self.numbones, self.boneindex,
            self.numbonecontrollers, self.bonecontrollerindex,
            self.numhitboxes, self.hitboxindex,
            self.numseq, self.seqindex,
            self.numseqgroups, self.seqgroupindex,
            self.numtextures, self.textureindex, self.texturedataindex,
            self.numskinref, self.numskinfamilies, self.skinindex,
            self.numbodyparts, self.bodypartindex,
            self.numattachments, self.attachmentindex,
            self.soundtable, self.soundindex, self.soundgroups, self.soundgroupindex,
            self.numtransitions, self.transitionindex,
        ) = struct.unpack_from("<26i", d, off)

    def bones(self):
        out = []
        for i in range(self.numbones):
            off = self.boneindex + i * BONE_SIZE
            f = struct.unpack_from(BONE_FMT, self.data, off)
            name = cstr(f[0])
            parent = f[1]
            flags = f[2]
            bonecontroller = f[3:9]
            value = f[9:15]
            scale = f[15:21]
            out.append({
                "name": name, "parent": parent, "flags": flags,
                "bonecontroller": bonecontroller, "value": value, "scale": scale,
            })
        return out

    def attachments(self):
        out = []
        bones = self.bones()
        for i in range(self.numattachments):
            off = self.attachmentindex + i * ATT_SIZE
            f = struct.unpack_from(ATT_FMT, self.data, off)
            name = cstr(f[0])
            bone = f[2]
            org = f[3:6]
            bname = bones[bone]["name"] if 0 <= bone < len(bones) else "?"
            out.append({"name": name, "bone": bone, "bone_name": bname, "org": org})
        return out

    def textures(self):
        out = []
        for i in range(self.numtextures):
            off = self.textureindex + i * TEXTURE_SIZE
            name_raw, flags, width, height, index = struct.unpack_from(TEXTURE_FMT, self.data, off)
            out.append({"name": cstr(name_raw), "flags": flags, "width": width,
                        "height": height, "index": index})
        return out

    def skinref(self):
        # short index[numskinfamilies][numskinref] ab skinindex
        n = self.numskinfamilies * self.numskinref
        vals = struct.unpack_from(f"<{n}h", self.data, self.skinindex)
        # nur die erste Skin-Familie fuer den Referenz-Export
        return list(vals[0:self.numskinref]) if self.numskinref else []

    def bodyparts(self):
        out = []
        for i in range(self.numbodyparts):
            off = self.bodypartindex + i * BODYPART_SIZE
            name_raw, nummodels, base, modelindex = struct.unpack_from(BODYPART_FMT, self.data, off)
            models = []
            for m in range(nummodels):
                moff = modelindex + m * MODEL_SIZE
                mf = struct.unpack_from(MODEL_FMT, self.data, moff)
                mname = cstr(mf[0])
                (mtype, boundingradius, nummesh, meshindex,
                 numverts, vertinfoindex, vertindex,
                 numnorms, norminfoindex, normindex,
                 numgroups, groupindex) = mf[1:]
                verts = list(struct.iter_unpack("<3f", self.data[vertindex:vertindex + numverts * 12]))
                verts = [v[0:3] for v in struct.iter_unpack("<3f", self.data[vertindex:vertindex + numverts * 12])]
                vertinfo = list(self.data[vertinfoindex:vertinfoindex + numverts])
                norms = [v for v in struct.iter_unpack("<3f", self.data[normindex:normindex + numnorms * 12])]
                norminfo = list(self.data[norminfoindex:norminfoindex + numnorms])
                meshes = []
                for me in range(nummesh):
                    meoff = meshindex + me * MESH_SIZE
                    numtris, triindex, skinref, numnorms_m, normindex_m = struct.unpack_from(
                        MESH_FMT, self.data, meoff
                    )
                    meshes.append({"numtris": numtris, "triindex": triindex, "skinref": skinref})
                models.append({
                    "name": mname, "numverts": numverts, "verts": verts, "vertinfo": vertinfo,
                    "numnorms": numnorms, "norms": norms, "norminfo": norminfo, "meshes": meshes,
                })
            out.append({"name": cstr(name_raw), "models": models})
        return out

    def sequences(self):
        out = []
        for i in range(self.numseq):
            off = self.seqindex + i * SEQ_SIZE
            f = struct.unpack_from(SEQ_FMT, self.data, off)
            label = cstr(f[0])
            fps = f[1]
            flags = f[2]
            numevents = f[5]
            eventindex = f[6]
            numframes = f[7]
            numblends = f[23]
            animindex = f[24]
            events = []
            for e in range(numevents):
                eoff = eventindex + e * EVENT_SIZE
                frame, event, etype, options = struct.unpack_from(EVENT_FMT, self.data, eoff)
                events.append({"frame": frame, "event": event, "type": etype, "options": cstr(options)})
            out.append({
                "index": i, "label": label, "fps": fps, "flags": flags,
                "numframes": numframes, "numblends": numblends, "animindex": animindex,
                "events": events,
            })
        return out

    def read_anim_value_stream(self, base_off, num_frames):
        """Dekodiert einen mstudioanimvalue_t-Strom (Positions- ODER
        Rotationskanal eines Bones) fuer alle Frames 0..num_frames-1.
        Algorithmus 1:1 aus CalcBonePosition/CalcBoneQuaternion
        (studio_render.cpp), ohne Zwischen-Frame-Interpolation (s=0,
        d.h. jeder Frame wird als exakter Integer-Frame ausgelesen)."""
        values = []
        d = self.data
        # Baue die Liste der "Spans" einmal komplett auf (num.valid, num.total, [raw int16 werte]).
        spans = []
        pos = base_off
        total_frames_covered = 0
        # Sicherheitsobergrenze, um Endlosschleifen bei defekten Daten zu vermeiden.
        guard = 0
        while total_frames_covered < num_frames and guard < 100000:
            guard += 1
            valid, total = struct.unpack_from("<BB", d, pos)
            pos += ANIMVALUE_SIZE
            raws = []
            for _ in range(valid):
                (raw,) = struct.unpack_from("<h", d, pos)
                raws.append(raw)
                pos += ANIMVALUE_SIZE
            spans.append((valid, total, raws))
            total_frames_covered += total
            if total == 0:
                break  # defekte/leere Spanne, Abbruch statt Endlosschleife

        # Jetzt fuer jeden Frame den passenden Wert extrahieren (kein Slerp, s=0 -> exakter Wert).
        for frame in range(num_frames):
            k = frame
            val = 0
            for (valid, total, raws) in spans:
                if k < total:
                    if valid > k:
                        val = raws[k]
                    elif raws:
                        val = raws[-1]
                    else:
                        val = 0
                    break
                k -= total
            else:
                val = raws[-1] if spans and spans[-1][2] else 0
            values.append(val)
        return values

    def decode_sequence_bones(self, seq):
        """Liefert je Frame eine Liste (pos[3], eulerRPY[3]) pro Bone,
        in Bone-lokalen Koordinaten relativ zum Elternbone (radians)."""
        bones = self.bones()
        numframes = max(seq["numframes"], 1)
        anim_base = self.seqindex + 0  # animindex ist bereits absolut? -> siehe unten
        animindex = seq["animindex"]
        per_bone = []
        for bi, bone in enumerate(bones):
            off = animindex + bi * ANIM_SIZE
            offsets = struct.unpack_from(ANIM_FMT, self.data, off)
            panim_base = off  # offsets sind relativ zum jeweiligen mstudioanim_t (also zu 'off')
            chans = []
            for ch in range(6):
                if offsets[ch] == 0:
                    chans.append([0] * numframes)
                else:
                    chans.append(self.read_anim_value_stream(panim_base + offsets[ch], numframes))
            per_bone.append(chans)

        frames_out = []
        for frame in range(numframes):
            frame_bones = []
            for bi, bone in enumerate(bones):
                chans = per_bone[bi]
                pos = [bone["value"][j] + chans[j][frame] * bone["scale"][j] for j in range(3)]
                rot = [bone["value"][j + 3] + chans[j + 3][frame] * bone["scale"][j + 3] for j in range(3)]
                frame_bones.append((tuple(pos), tuple(rot)))
            frames_out.append(frame_bones)
        return frames_out


# ---------------------------------------------------------------------------
# SMD-/QC-Export
# ---------------------------------------------------------------------------

def write_skeleton_smd(path, bones, frames, nodes_only_frame0=False, triangles=None):
    """frames: Liste von Frames, jeder Frame ist eine Liste
    (pos[3], eulerRPY[3]) pro Bone-Index (Bone-lokal, wie im SMD-Standard)."""
    with open(path, "w", encoding="utf-8") as f:
        f.write("version 1\n")
        f.write("nodes\n")
        for i, b in enumerate(bones):
            f.write(f"{i} \"{b['name']}\" {b['parent']}\n")
        f.write("end\n")
        f.write("skeleton\n")
        for t, frame_bones in enumerate(frames):
            f.write(f"time {t}\n")
            for i, (pos, rot) in enumerate(frame_bones):
                f.write(f"{i}  {pos[0]:.6f} {pos[1]:.6f} {pos[2]:.6f}  "
                        f"{rot[0]:.6f} {rot[1]:.6f} {rot[2]:.6f}\n")
        f.write("end\n")
        if triangles is not None:
            f.write("triangles\n")
            for tri in triangles:
                f.write(tri)
            f.write("end\n")


def expand_tricmds(mdl, triindex, tex_by_skinref, tex_width_height):
    """Liest die Dreiecks-Befehlsliste (Streifen/Faecher) und liefert eine
    flache Liste von (vertindex, normindex, s_px, t_px) Dreiecks-Tripeln."""
    d = mdl.data
    pos = triindex
    triangles = []
    while True:
        (count,) = struct.unpack_from("<h", d, pos)
        pos += 2
        if count == 0:
            break
        is_fan = count < 0
        n = abs(count)
        verts = []
        for _ in range(n):
            vidx, nidx, s, t = struct.unpack_from("<hhhh", d, pos)
            pos += 8
            verts.append((vidx, nidx, s, t))
        if is_fan:
            for i in range(1, n - 1):
                triangles.append((verts[0], verts[i], verts[i + 1]))
        else:
            for i in range(n - 2):
                if i % 2 == 0:
                    triangles.append((verts[i], verts[i + 1], verts[i + 2]))
                else:
                    triangles.append((verts[i + 1], verts[i], verts[i + 2]))
    return triangles, pos


def decompile(mdl_path, out_dir, do_anims=True):
    mdl = Mdl(mdl_path)
    if mdl.id != "IDST" or mdl.version != 10:
        print(f"WARNUNG: {mdl_path}: id={mdl.id!r} version={mdl.version} "
              f"(erwartet IDST/10) -- evtl. keine Haupt-.mdl-Datei.")

    base = os.path.splitext(os.path.basename(mdl_path))[0]
    os.makedirs(out_dir, exist_ok=True)
    tex_dir = os.path.join(out_dir, "textures")
    os.makedirs(tex_dir, exist_ok=True)

    # --- Texturen ---
    tex_names = []
    for i, tex in enumerate(mdl.textures()):
        w, h, idx = tex["width"], tex["height"], tex["index"]
        pixdata = mdl.data[idx:idx + w * h]
        pal_off = idx + w * h
        palbytes = mdl.data[pal_off:pal_off + 256 * 3]
        if len(pixdata) < w * h or len(palbytes) < 256 * 3:
            print(f"  WARNUNG: Textur {tex['name']!r} unvollstaendig, uebersprungen.")
            continue
        palette = [tuple(palbytes[j * 3:j * 3 + 3]) for j in range(256)]
        safe_name = "".join(c if c.isalnum() or c in "._-" else "_" for c in tex["name"])
        if not safe_name.lower().endswith(".bmp"):
            safe_name_bmp = os.path.splitext(safe_name)[0] + ".bmp"
        else:
            safe_name_bmp = safe_name
        write_bmp_indexed(os.path.join(tex_dir, safe_name_bmp), w, h, pixdata, palette)
        tex_names.append(safe_name_bmp)

    bones = mdl.bones()
    attachments = mdl.attachments()
    bodyparts = mdl.bodyparts()
    sequences = mdl.sequences()
    skinref = mdl.skinref()
    textures = mdl.textures()

    # --- Rest-Pose-Weltmatrizen je Bone (fuer Referenz-Mesh) ---
    world = [None] * len(bones)
    for i, b in enumerate(bones):
        pos = b["value"][0:3]
        quat = angle_quaternion(b["value"][3], b["value"][4], b["value"][5])
        local = make_bone_matrix(pos, quat)
        if b["parent"] == -1:
            world[i] = local
        else:
            world[i] = concat_transforms(world[b["parent"]], local)

    # --- Referenz-SMD (Bind-Pose-Geometrie) ---
    ref_frame = [((0.0, 0.0, 0.0), (0.0, 0.0, 0.0))] * 0
    ref_frame_bones = []
    for b in bones:
        ref_frame_bones.append((tuple(b["value"][0:3]), tuple(b["value"][3:6])))

    tri_lines = []
    for bp in bodyparts:
        for model in bp["models"]:
            verts_world = []
            for vi in range(model["numverts"]):
                bone_i = model["vertinfo"][vi]
                verts_world.append(vector_transform(model["verts"][vi], world[bone_i]))
            norms_world = []
            for ni in range(model["numnorms"]):
                bone_i = model["norminfo"][ni]
                norms_world.append(vector_rotate(model["norms"][ni], world[bone_i]))

            for mesh in model["meshes"]:
                sr = mesh["skinref"]
                tex_idx = skinref[sr] if sr < len(skinref) else sr
                if tex_idx >= len(textures):
                    tex_idx = 0
                tex = textures[tex_idx] if textures else None
                tex_bmp = tex_names[tex_idx] if tex and tex_idx < len(tex_names) else "notexture.bmp"
                tw = tex["width"] if tex else 1
                th = tex["height"] if tex else 1
                tris, _ = expand_tricmds(mdl, mesh["triindex"], None, None)
                for (v0, v1, v2) in tris:
                    line = f"{tex_bmp}\n"
                    for (vidx, nidx, s, t) in (v0, v1, v2):
                        wx, wy, wz = verts_world[vidx]
                        nx, ny, nz = norms_world[nidx] if norms_world else (0.0, 0.0, 1.0)
                        u = s / tw if tw else 0.0
                        v = 1.0 - (t / th if th else 0.0)
                        bone_i = model["vertinfo"][vidx]
                        line += (f"{bone_i}  {wx:.6f} {wy:.6f} {wz:.6f}  "
                                 f"{nx:.6f} {ny:.6f} {nz:.6f}  {u:.6f} {v:.6f}\n")
                    tri_lines.append(line)

    ref_path = os.path.join(out_dir, base + "_ref.smd")
    write_skeleton_smd(ref_path, bones, [ref_frame_bones], triangles=tri_lines)
    print(f"  Referenz-SMD: {ref_path} ({len(tri_lines)} Dreiecke)")

    # --- Sequenz-SMDs (Animation, ohne Geometrie) ---
    seq_files = []
    if do_anims:
        for seq in sequences:
            try:
                frames = mdl.decode_sequence_bones(seq)
            except (struct.error, IndexError) as e:
                print(f"  WARNUNG: Sequenz {seq['label']!r} konnte nicht dekodiert werden: {e}")
                continue
            safe_label = "".join(c if c.isalnum() or c in "._-" else "_" for c in seq["label"])
            seq_path = os.path.join(out_dir, f"{base}_seq_{seq['index']:02d}_{safe_label}.smd")
            write_skeleton_smd(seq_path, bones, frames)
            seq_files.append((seq, seq_path))
        print(f"  {len(seq_files)}/{len(sequences)} Animationssequenzen exportiert.")

    # --- QC-Datei ---
    qc_path = os.path.join(out_dir, base + ".qc")
    with open(qc_path, "w", encoding="utf-8") as f:
        f.write(f'$modelname "{base}.mdl"\n')
        f.write(f'$cd "."\n')
        f.write(f'$cdtexture "textures"\n')
        f.write(f'$scale 1.0\n\n')
        seen_tex = set()
        for t in tex_names:
            if t not in seen_tex:
                seen_tex.add(t)
        f.write(f'$bodygroup "studio"\n{{\n  studio "{base}_ref.smd"\n}}\n\n')
        for att in attachments:
            f.write(f'$attachment {att["bone"]} '
                    f'"{att["bone_name"]}" {att["org"][0]:.4f} {att["org"][1]:.4f} {att["org"][2]:.4f}\n')
        f.write("\n")
        for seq, seq_path in (seq_files if do_anims else []):
            fname = os.path.basename(seq_path)
            loop = " loop" if seq["flags"] & 1 else ""
            f.write(f'$sequence "{seq["label"]}" "{fname}" fps {seq["fps"]:.2f}{loop}\n')
    print(f"  QC-Datei: {qc_path}")
    print(f"  HINWEIS: Referenz-Geometrie ist nicht visuell gegengeprueft (siehe Modul-Docstring) --")
    print(f"  vor produktiver Nutzung in Blender/studiomdl gegenchecken.")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mdl", help="Pfad zur .mdl-Hauptdatei")
    ap.add_argument("out_dir", help="Zielordner fuer die dekompilierten Quelldateien")
    ap.add_argument("--no-anims", action="store_true", help="Nur Referenz-Mesh, keine Animationssequenzen exportieren")
    args = ap.parse_args()

    if not os.path.isfile(args.mdl):
        print(f"Datei nicht gefunden: {args.mdl}", file=sys.stderr)
        sys.exit(1)

    print(f"=== {args.mdl} -> {args.out_dir} ===")
    decompile(args.mdl, args.out_dir, do_anims=not args.no_anims)


if __name__ == "__main__":
    main()
