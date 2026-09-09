#!/usr/bin/env python3
"""
GoldSrc-Studiomodell-Inspektor (.mdl, Version 10) fuer die Gunman-Chronicles-
Reverse-Engineering-Doku.

Liest Bones, Attachment-Punkte, Body-Parts/Sub-Modelle und
Animations-Sequenzen samt darin eingebetteter QC-Events (Frame, Event-Code,
Options-String) DIREKT aus der Binaerdatei aus -- ohne externe Tools
(Crowbar/HLMV/Wine).

Die Struct-Layouts sind wortwoertlich aus
`re-project/src/halflife-updated-gm/engine/studio.h` und
`.../engine/studio_event.h` uebernommen (demselben Engine-Fork, den Gunman
Chronicles nutzt) -- keine geschaetzten/generischen GoldSrc-Definitionen,
sondern die tatsaechlich fuer dieses Spiel gueltigen Strukturen. Damit lassen
sich Events (z.B. "events/batbotbeam.sc"-Ausloeser) und Attachment-Punkte
(z.B. "3 Beine mit Beam zur Mitte") direkt im Modell nachvollziehen, ohne auf
eine Vermutung aus der DLL-Decompilation angewiesen zu sein.

Nutzung:
    python3 mdl_inspect.py <pfad/zu/modell.mdl> [--events] [--attachments]
                                                  [--bones] [--bodyparts] [--all]

Ohne Flags wird eine kompakte Zusammenfassung ausgegeben. Mit --all werden
alle Details ausgegeben (empfohlen fuer die erste Untersuchung einer neuen
Entity).

Hinweis: Externe (getrennte) Sequenzgruppen-Dateien (z.B. "modellname01.mdl"
bei numseqgroups > 1) werden fuer Events/Attachments NICHT benoetigt -- diese
Daten liegen immer in der Haupt-.mdl-Datei. Nur die eigentlichen
Animations-Frame-Daten koennen ausgelagert sein, sind fuer diese
Untersuchung aber irrelevant.
"""
import struct
import sys
import argparse
import os


def cstr(b):
    return b.split(b"\x00", 1)[0].decode("latin-1", errors="replace")


class StudioModel:
    def __init__(self, path):
        with open(path, "rb") as f:
            self.data = f.read()
        self._parse_header()

    def _parse_header(self):
        d = self.data
        (
            self.id, self.version,
        ) = struct.unpack_from("<4s i", d, 0)
        self.id = self.id.decode("latin-1", errors="replace")
        name_raw, self.length = struct.unpack_from("<64s i", d, 8)
        self.name = cstr(name_raw)
        off = 8 + 64 + 4
        (
            self.eyeposition, self.min, self.max, self.bbmin, self.bbmax,
        ) = [struct.unpack_from("<3f", d, off + 12 * i) for i in range(5)]
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

    def check(self):
        if self.id != "IDST":
            print(f"WARNUNG: Datei-Magic ist {self.id!r}, erwartet 'IDST' "
                  f"(evtl. eine Sequenzgruppen-/Textur-Zusatzdatei statt des "
                  f"Hauptmodells, oder kein GoldSrc-v10-Modell).")
        if self.version != 10:
            print(f"WARNUNG: Version {self.version}, erwartet 10 (GoldSrc). "
                  f"Struct-Layouts koennten nicht mehr passen.")

    # ---- Bones ----
    BONE_FMT = "<32s i i 6i 6f 6f"
    BONE_SIZE = struct.calcsize(BONE_FMT)

    def bones(self):
        out = []
        for i in range(self.numbones):
            off = self.boneindex + i * self.BONE_SIZE
            fields = struct.unpack_from(self.BONE_FMT, self.data, off)
            name = cstr(fields[0])
            parent = fields[1]
            out.append((i, name, parent))
        return out

    # ---- Attachments ----
    ATT_FMT = "<32s i i 3f 9f"
    ATT_SIZE = struct.calcsize(ATT_FMT)

    def attachments(self):
        out = []
        for i in range(self.numattachments):
            off = self.attachmentindex + i * self.ATT_SIZE
            fields = struct.unpack_from(self.ATT_FMT, self.data, off)
            name = cstr(fields[0])
            atype = fields[1]
            bone = fields[2]
            org = fields[3:6]
            bone_name = "?"
            bones = self.bones()
            if 0 <= bone < len(bones):
                bone_name = bones[bone][1]
            out.append((i, name, atype, bone, bone_name, org))
        return out

    # ---- Bodyparts / Models ----
    BODYPART_FMT = "<64s i i i"
    BODYPART_SIZE = struct.calcsize(BODYPART_FMT)

    def bodyparts(self):
        out = []
        for i in range(self.numbodyparts):
            off = self.bodypartindex + i * self.BODYPART_SIZE
            name_raw, nummodels, base, modelindex = struct.unpack_from(
                self.BODYPART_FMT, self.data, off
            )
            out.append((i, cstr(name_raw), nummodels, base, modelindex))
        return out

    # ---- Sequences + Events ----
    SEQ_FMT = "<32s f i i i i i i i i i i 3f i i 3f 3f i i 2i 2f 2f i i i i i i"
    SEQ_SIZE = struct.calcsize(SEQ_FMT)
    EVENT_FMT = "<i i i 64s"
    EVENT_SIZE = struct.calcsize(EVENT_FMT)

    def sequences(self):
        out = []
        for i in range(self.numseq):
            off = self.seqindex + i * self.SEQ_SIZE
            f = struct.unpack_from(self.SEQ_FMT, self.data, off)
            label = cstr(f[0])
            fps = f[1]
            flags = f[2]
            activity = f[3]
            actweight = f[4]
            numevents = f[5]
            eventindex = f[6]
            numframes = f[7]
            events = []
            for e in range(numevents):
                eoff = eventindex + e * self.EVENT_SIZE
                frame, event, etype, options = struct.unpack_from(
                    self.EVENT_FMT, self.data, eoff
                )
                events.append((frame, event, etype, cstr(options)))
            out.append({
                "index": i, "label": label, "fps": fps, "flags": flags,
                "activity": activity, "actweight": actweight,
                "numframes": numframes, "events": events,
            })
        return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mdl", help="Pfad zur .mdl-Datei (Hauptmodell, nicht 'T.mdl'/Sequenzgruppen-Datei)")
    ap.add_argument("--events", action="store_true", help="Alle Sequenzen + eingebettete Events auflisten")
    ap.add_argument("--attachments", action="store_true", help="Attachment-Punkte auflisten")
    ap.add_argument("--bones", action="store_true", help="Bone-Hierarchie auflisten")
    ap.add_argument("--bodyparts", action="store_true", help="Body-Parts/Sub-Modelle auflisten")
    ap.add_argument("--all", action="store_true", help="Alles auflisten")
    args = ap.parse_args()

    if not os.path.isfile(args.mdl):
        print(f"Datei nicht gefunden: {args.mdl}", file=sys.stderr)
        sys.exit(1)

    m = StudioModel(args.mdl)
    m.check()

    print(f"=== {m.name or os.path.basename(args.mdl)} (MDL v{m.version}) ===")
    print(f"Bones: {m.numbones}  Attachments: {m.numattachments}  "
          f"BodyParts: {m.numbodyparts}  Sequenzen: {m.numseq}  "
          f"Sequenzgruppen: {m.numseqgroups}  Texturen: {m.numtextures}")
    print(f"Bounding-Box (Bewegungshuelle): min={tuple(round(x,1) for x in m.min)} "
          f"max={tuple(round(x,1) for x in m.max)}")
    print(f"Clipping-BBox: min={tuple(round(x,1) for x in m.bbmin)} "
          f"max={tuple(round(x,1) for x in m.bbmax)}")

    show_all = args.all
    if args.bones or show_all:
        print("\n--- Bones ---")
        for i, name, parent in m.bones():
            pname = m.bones()[parent][1] if 0 <= parent < m.numbones else "(root)"
            print(f"  [{i:3d}] {name:<24s} parent={pname}")

    if args.attachments or show_all:
        print("\n--- Attachment-Punkte ---")
        if m.numattachments == 0:
            print("  (keine)")
        for i, name, atype, bone, bone_name, org in m.attachments():
            print(f"  [{i}] name={name!r:20s} bone={bone} ({bone_name})  "
                  f"org={tuple(round(x,2) for x in org)}")

    if args.bodyparts or show_all:
        print("\n--- Body-Parts / Sub-Modelle ---")
        for i, name, nummodels, base, modelindex in m.bodyparts():
            print(f"  [{i}] {name!r:20s} nummodels={nummodels}")

    if args.events or show_all:
        print("\n--- Sequenzen + eingebettete Events ---")
        for seq in m.sequences():
            print(f"  [{seq['index']:3d}] {seq['label']:<24s} "
                  f"fps={seq['fps']:.1f} frames={seq['numframes']} "
                  f"activity={seq['activity']} events={len(seq['events'])}")
            for frame, event, etype, options in seq["events"]:
                print(f"        frame={frame:4d}  event={event:6d}  "
                      f"type={etype}  options={options!r}")

    if not (args.events or args.attachments or args.bones or args.bodyparts or show_all):
        print("\n(Nutze --events/--attachments/--bones/--bodyparts/--all fuer Details)")


if __name__ == "__main__":
    main()
