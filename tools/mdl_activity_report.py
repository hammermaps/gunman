#!/usr/bin/env python3
"""Erzeugt eine maschinenlesbare Animations-Uebersicht (CSV) je .mdl-Datei:
Sequenzname, Laenge (Sekunden = Frames/FPS), Frames, FPS, aufgeloester
ACT_*-Aktivitaetstyp und Event-Anzahl.

Zweck: Referenz fuer die spaetere C++-Neuimplementierung der Monster-KI --
die Schedule-/Task-Systeme des HL-SDKs (siehe `re-project/src/halflife-
updated-gm/dlls/{activity.h,activitymap.h,schedule.h}`) planen Verhalten
in Aktivitaeten (ACT_IDLE, ACT_MELEE_ATTACK1, ...) und muessen die
tatsaechliche Animationsdauer kennen (z.B. um Angriffs-Timing/Refeuer-Reize
mit der Animation zu synchronisieren).

Die ACT_*-Tabelle unten ist wortwoertlich aus
`re-project/src/halflife-updated-gm/dlls/activity.h` uebernommen (gleicher
Engine-Fork wie bei allen anderen Tools dieses Projekts) -- Reihenfolge und
Werte sind fest (C-enum, bei 0 beginnend), NICHT geschaetzt.

Nutzung:
    python3 mdl_activity_report.py <modell.mdl> [<ausgabe.csv>]
    python3 mdl_activity_report.py --batch <models-src-ordner> <ausgabe.csv>

Ohne Ausgabepfad wird nach <modellname>_animations.csv im selben Ordner wie
das Modell geschrieben.
"""
import csv
import glob
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mdl_inspect import StudioModel  # noqa: E402

# Woertlich aus dlls/activity.h (Activity-Enum), Index = numerischer Wert.
ACTIVITY_NAMES = [
    "ACT_RESET", "ACT_IDLE", "ACT_GUARD", "ACT_WALK", "ACT_RUN", "ACT_FLY",
    "ACT_SWIM", "ACT_HOP", "ACT_LEAP", "ACT_FALL", "ACT_LAND",
    "ACT_STRAFE_LEFT", "ACT_STRAFE_RIGHT", "ACT_ROLL_LEFT", "ACT_ROLL_RIGHT",
    "ACT_TURN_LEFT", "ACT_TURN_RIGHT", "ACT_CROUCH", "ACT_CROUCHIDLE",
    "ACT_STAND", "ACT_USE", "ACT_SIGNAL1", "ACT_SIGNAL2", "ACT_SIGNAL3",
    "ACT_TWITCH", "ACT_COWER", "ACT_SMALL_FLINCH", "ACT_BIG_FLINCH",
    "ACT_RANGE_ATTACK1", "ACT_RANGE_ATTACK2", "ACT_MELEE_ATTACK1",
    "ACT_MELEE_ATTACK2", "ACT_RELOAD", "ACT_ARM", "ACT_DISARM", "ACT_EAT",
    "ACT_DIESIMPLE", "ACT_DIEBACKWARD", "ACT_DIEFORWARD", "ACT_DIEVIOLENT",
    "ACT_BARNACLE_HIT", "ACT_BARNACLE_PULL", "ACT_BARNACLE_CHOMP",
    "ACT_BARNACLE_CHEW", "ACT_SLEEP", "ACT_INSPECT_FLOOR",
    "ACT_INSPECT_WALL", "ACT_IDLE_ANGRY", "ACT_WALK_HURT", "ACT_RUN_HURT",
    "ACT_HOVER", "ACT_GLIDE", "ACT_FLY_LEFT", "ACT_FLY_RIGHT",
    "ACT_DETECT_SCENT", "ACT_SNIFF", "ACT_BITE", "ACT_THREAT_DISPLAY",
    "ACT_FEAR_DISPLAY", "ACT_EXCITED", "ACT_SPECIAL_ATTACK1",
    "ACT_SPECIAL_ATTACK2", "ACT_COMBAT_IDLE", "ACT_WALK_SCARED",
    "ACT_RUN_SCARED", "ACT_VICTORY_DANCE", "ACT_DIE_HEADSHOT",
    "ACT_DIE_CHESTSHOT", "ACT_DIE_GUTSHOT", "ACT_DIE_BACKSHOT",
    "ACT_FLINCH_HEAD", "ACT_FLINCH_CHEST", "ACT_FLINCH_STOMACH",
    "ACT_FLINCH_LEFTARM", "ACT_FLINCH_RIGHTARM", "ACT_FLINCH_LEFTLEG",
    "ACT_FLINCH_RIGHTLEG",
]


def activity_name(act_id):
    if act_id == 0:
        return "ACT_RESET (kein Activity-Tag / 0)"
    if 0 <= act_id < len(ACTIVITY_NAMES):
        return ACTIVITY_NAMES[act_id]
    return f"ACT_UNKNOWN({act_id})"


def collect_rows(mdl_path):
    model_name = os.path.splitext(os.path.basename(mdl_path))[0]
    sm = StudioModel(mdl_path)
    rows = []
    for seq in sm.sequences():
        fps = seq["fps"] if seq["fps"] > 0 else 1.0
        duration = seq["numframes"] / fps
        rows.append({
            "modell": model_name,
            "sequenz_index": seq["index"],
            "sequenz_name": seq["label"],
            "fps": round(seq["fps"], 3),
            "frames": seq["numframes"],
            "dauer_sekunden": round(duration, 4),
            "activity_id": seq["activity"],
            "activity_name": activity_name(seq["activity"]),
            "activity_gewicht": seq["actweight"],
            "anzahl_events": len(seq["events"]),
            "loop": "flags&1" if (seq["flags"] & 1) else "",
        })
    return rows


FIELDNAMES = [
    "modell", "sequenz_index", "sequenz_name", "fps", "frames",
    "dauer_sekunden", "activity_id", "activity_name", "activity_gewicht",
    "anzahl_events", "loop",
]


def write_csv(rows, out_path):
    with open(out_path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=FIELDNAMES)
        w.writeheader()
        w.writerows(rows)


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__, file=sys.stderr)
        sys.exit(1)

    if args[0] == "--batch":
        src_dir, out_path = args[1], args[2]
        all_rows = []
        mdl_files = sorted(glob.glob(os.path.join(src_dir, "*", "*.mdl")))
        failed = 0
        for mdl_path in mdl_files:
            try:
                rows = collect_rows(mdl_path)
                all_rows.extend(rows)
                per_model_out = os.path.join(
                    os.path.dirname(mdl_path),
                    os.path.splitext(os.path.basename(mdl_path))[0] + "_animations.csv")
                write_csv(rows, per_model_out)
            except Exception as e:
                print(f"FEHLER bei {mdl_path}: {e}", file=sys.stderr)
                failed += 1
        write_csv(all_rows, out_path)
        print(f"{len(mdl_files) - failed}/{len(mdl_files)} Modelle "
              f"(je 1 <modell>_animations.csv im Modellordner), "
              f"{len(all_rows)} Sequenzen gesamt -> {out_path}")
        return

    mdl_path = args[0]
    out_path = args[1] if len(args) > 1 else (
        os.path.join(os.path.dirname(mdl_path),
                      os.path.splitext(os.path.basename(mdl_path))[0] + "_animations.csv"))
    rows = collect_rows(mdl_path)
    write_csv(rows, out_path)
    print(f"{len(rows)} Sequenzen -> {out_path}")


if __name__ == "__main__":
    main()
