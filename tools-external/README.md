# Drittanbieter-Referenzprojekte (lokale Snapshots)

Diese Unterordner sind lokale Kopien externer, öffentlicher
GitHub-Projekte, die als **Cross-Referenz** für die RE-Arbeit dienen
(siehe `re-project/references.md` für die jeweilige Bewertung/Findings-
Datei). **Kein Ersatz für die eigene Decompilation** — Nutzungsregeln
stehen in den jeweiligen `findings/*_crossref.md`-Dateien.

**Hinweis zur Beschaffung:** `git clone` gegen `github.com` schlägt in
dieser Sandbox-Umgebung fehl (`git`s Smart-HTTP-Protokoll wird
blockiert/verlangt einen Auth-Prompt, obwohl die Repos öffentlich
sind), einfache HTTPS-GETs (`curl`, `codeload.github.com`) funktionieren
aber einwandfrei. Deshalb sind dies **Tarball-Snapshots** des jeweils
aktuellen `master`-Branches zum unten genannten Commit, keine lebenden
Git-Arbeitskopien (kein `.git`-Verzeichnis). Bei Bedarf erneut per
`curl -L https://codeload.github.com/<owner>/<repo>/tar.gz/refs/heads/master`
aktualisieren.

## `HalfLife.UnifiedSdk.MapDecompiler`

Siehe `re-project/tools-setup.md` (Session 47) — separat behandelt, kein
Snapshot-Hinweis hier nötig.

## `freegunman/`

- Quelle: https://github.com/eukara/freegunman
- Snapshot-Commit: `c1457896fdc5848e2b154184b9fa3abb42d4e898` (2024-03-02)
- Lizenz: ISC (siehe `LICENSE` im Ordner)
- Vollständig heruntergeladen (183 KB, keine großen Asset-Binärdateien
  enthalten) — echter QuakeC-Sourcecode für FTEQW/Nuclide-SDK.
- Bewertung/Cross-Referenz-Befunde: `findings/freegunman_crossref.md`

## `SvenCoop-GC/`

- Quelle: https://github.com/MisterCalvin/SvenCoop-GC
- Snapshot-Commit: `d3dc76c9272dc3ca5e81449bef74e946a1246e9e` (2023-07-18)
- **Nur `scripts/` + `README.md` extrahiert** (65 KB) — das Original-
  Repo ist ~51 MB groß, davon >600 Dateien reine Asset-Duplikate
  (`.wad`/`.bsp`/`.mdl`/`.tga`/`.wav`/`.spr`), die wir bereits im
  Original besitzen (`Gunman-ENG-GER/`). Nur die 51
  AngelScript-Dateien (`.as`) unter `scripts/maps/gunmanchronicles/`
  sind für uns als Code-Referenz relevant.
- Bewertung/Cross-Referenz-Befunde: `findings/svencoop_gc_crossref.md`

## `HLSourceHub/goldsrc-gunman_chronicles` — bewusst NICHT geklont

Enthält laut Prüfung (siehe `references.md`) keinen Sourcecode, nur ein
Steam-Mod-Ordner-Layout mit den kompilierten Original-DLLs und
Original-Assets (identisch zu unserem bereits vorhandenen
`Gunman-ENG-GER/`-Bestand) — ein lokaler Klon würde nur bereits
vorhandene Dateien duplizieren. Bei Bedarf (z. B. als
Paketierungs-Vorlage, siehe `findings/rebuild_ideas.md`) gezielt nur
die Konfigurationsdateien (`liblist.gam`, `*.cfg`, `*_textscheme.txt`)
nachträglich per `curl` holen.

## `VHLT-V34/`

- Quelle: https://github.com/twhl-community/VHLT-V34 (Commit
  `1dd2c2f2dbec06a66acf1b417756f671e055dc48`)
- **Kein Referenzprojekt für RE-Cross-Reference**, sondern ein
  echtes Build-Tool: Vluzacn's ZHLT v34, ein weit verbreiteter,
  langjährig gepflegter Custom-Build von Zoner's Half-Life
  Compilation Tools v3.4 (`hlcsg`/`hlbsp`/`hlvis`/`hlrad`/`ripent`,
  unter `src/zhlt-vluzacn/`), mit einem bereits funktionierenden,
  explizit für Linux/GNU geschriebenen Makefile (im Gegensatz zum
  Original `kriswema/zhlt`, das reines MSVC/Windows-Projekt ist).
  Bringt `zhlt.fgd`/`zhlt.wad` (Editor-Hilfsdateien für Hammer/
  J.A.C.K.) direkt im `tools/`-Unterordner mit — nützlich, da die
  ursprüngliche `zhlt.info`-Quelle für diese Dateien offline ist.
  Wird per `scripts/build-zhlt-linux.sh` gebaut/bei Bedarf neu
  geklont (deshalb als lebende Git-Arbeitskopie behalten, nicht
  als Tarball-Snapshot wie die übrigen Ordner hier).
- Details/Fallstricke beim Erzeugen eigener `.map`-Dateien:
  `tools-setup.md` (Abschnitt "VHLT-V34").
