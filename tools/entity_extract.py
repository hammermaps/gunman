#!/usr/bin/env python3
"""Extrahiert GoldSrc-Entity-Text (.ent, wie von bsp_decompile.py erzeugt)
in ein durchsuchbares Markdown-Dokument je Karte: alle Entities mit allen
Keyvalues, plus automatisch aufgeloeste Verknuepfungen zwischen Entities
(targetname-Referenzen).

Verknuepfungs-Heuristik (keine feste Liste von Schluesselwoertern noetig,
deckt damit auch Gunman-eigene Custom-Keys wie "m_iszEntity" ab):
  - Fuer jede Karte wird zunaechst ein Index targetname -> [Entity-Indizes]
    aufgebaut.
  - Fuer jede Entity wird dann jeder Keyvalue (ausser "targetname" selbst)
    daraufhin geprueft, ob sein *Wert* exakt einem bekannten targetname in
    derselben Karte entspricht -> das zaehlt als ausgehende Verknuepfung.
  - Sonderfall multi_manager: dort sind die Keys selbst Ziel-targetnames
    (Value = Verzoegerung in Sekunden), siehe Half-Life-SDK.

Nur reines Python (struct/re/collections), keine externen Abhaengigkeiten.
"""
import re
import sys
from pathlib import Path
from collections import defaultdict, Counter

KV_RE = re.compile(r'"((?:[^"\\]|\\.)*)"\s*"((?:[^"\\]|\\.)*)"')


def parse_ents(text):
    """Parst den GoldSrc-Entity-Klartext in eine Liste von Entities.

    Jede Entity ist eine Liste von (key, value)-Paaren in Original-
    reihenfolge (Duplikate wie doppeltes "classname" in worldspawn bleiben
    erhalten -- der Engine-Konvention nach gewinnt der letzte Wert).
    """
    entities = []
    depth = 0
    current = []
    for line in text.splitlines():
        stripped = line.strip()
        if stripped == "{":
            depth += 1
            current = []
            continue
        if stripped == "}":
            depth -= 1
            if current:
                entities.append(current)
            continue
        if depth == 1:
            m = KV_RE.match(stripped)
            if m:
                current.append((m.group(1), m.group(2)))
    return entities


def kv_dict(pairs):
    """Letzter Wert gewinnt bei doppelten Keys (Engine-Konvention)."""
    d = {}
    for k, v in pairs:
        d[k] = v
    return d


def build_document(map_name, entities):
    n = len(entities)
    dicts = [kv_dict(pairs) for pairs in entities]
    classnames = [d.get("classname", "<ohne classname>") for d in dicts]

    # targetname -> [Entity-Indizes]
    targetname_index = defaultdict(list)
    for i, d in enumerate(dicts):
        tn = d.get("targetname")
        if tn:
            targetname_index[tn].append(i)

    known_targetnames = set(targetname_index.keys())

    # Ausgehende Verknuepfungen je Entity: [(key, value, [ziel-indizes])]
    outgoing = defaultdict(list)
    incoming = defaultdict(list)

    for i, pairs in enumerate(entities):
        cls = classnames[i]
        if cls == "multi_manager":
            # Sonderfall: Key = Ziel-targetname, Value = Verzoegerung (Sekunden)
            for k, v in pairs:
                if k in ("classname", "targetname", "origin", "spawnflags"):
                    continue
                if k in known_targetnames:
                    targets = targetname_index[k]
                    outgoing[i].append((k, f"Verzoegerung {v}s", targets))
                    for t in targets:
                        incoming[t].append((i, k))
        else:
            for k, v in pairs:
                if k == "targetname":
                    continue
                if v in known_targetnames:
                    # Selbstreferenzen (eigener targetname zufaellig als Wert)
                    # sind kein sinnvoller Link -- ausschliessen.
                    targets = [t for t in targetname_index[v] if t != i]
                    if targets:
                        outgoing[i].append((k, v, targets))
                        for t in targets:
                            incoming[t].append((i, k))

    lines = []
    lines.append(f"# Entity-Uebersicht: `{map_name}`\n")
    lines.append(f"Automatisch erzeugt von `tools/entity_extract.py` aus `{map_name}.ent`. "
                  f"Verknuepfungen wurden per targetname-Abgleich aufgeloest (siehe Skript-Docstring).\n")
    lines.append(f"**{n} Entities** insgesamt, **{len(known_targetnames)} eindeutige targetnames**.\n")

    lines.append("## Klassennamen-Haeufigkeit\n")
    lines.append("| Classname | Anzahl |")
    lines.append("|---|---:|")
    for cls, count in Counter(classnames).most_common():
        lines.append(f"| `{cls}` | {count} |")
    lines.append("")

    lines.append("## Entities\n")
    for i, pairs in enumerate(entities):
        d = dicts[i]
        cls = classnames[i]
        tn = d.get("targetname")
        header = f"### <a id=\"ent{i}\"></a>#{i} — `{cls}`"
        if tn:
            header += f" (targetname: `{tn}`)"
        lines.append(header)
        lines.append("")
        lines.append("| Key | Value |")
        lines.append("|---|---|")
        for k, v in pairs:
            v_escaped = v.replace("|", "\\|")
            lines.append(f"| `{k}` | {v_escaped} |")
        lines.append("")

        if outgoing[i]:
            lines.append("**Verknuepft mit (ausgehend):**")
            for k, v, targets in outgoing[i]:
                for t in targets:
                    tcls = classnames[t]
                    lines.append(f"- `{k}` = `{v}` &rarr; [#{t} `{tcls}`](#ent{t})")
            lines.append("")

        if incoming[i]:
            lines.append("**Referenziert von (eingehend):**")
            for src, k in incoming[i]:
                scls = classnames[src]
                lines.append(f"- [#{src} `{scls}`](#ent{src}) ueber `{k}`")
            lines.append("")

    return "\n".join(lines)


def extract(ent_path: Path, out_path: Path):
    text = ent_path.read_text(encoding="latin-1")
    entities = parse_ents(text)
    map_name = ent_path.stem
    doc = build_document(map_name, entities)
    out_path.write_text(doc, encoding="utf-8")
    return len(entities)


def main():
    if len(sys.argv) != 3:
        print("Nutzung: entity_extract.py <karte.ent> <ausgabe.md>", file=sys.stderr)
        sys.exit(1)
    ent_path = Path(sys.argv[1])
    out_path = Path(sys.argv[2])
    n = extract(ent_path, out_path)
    print(f"{ent_path.name}: {n} Entities -> {out_path}")


if __name__ == "__main__":
    main()
