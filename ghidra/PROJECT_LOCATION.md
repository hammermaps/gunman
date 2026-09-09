# Ghidra-Projekt — Speicherort

**Wichtig:** Das eigentliche Ghidra-Projekt liegt **außerhalb** dieses
Spielordners:

```
/home/masterbee/ghidra-projects/GunmanChronicles.gpr
/home/masterbee/ghidra-projects/GunmanChronicles.rep/
```

**Grund:** Ghidras `ProjectLocator` verbietet jede Pfadkomponente, die mit
einem Punkt beginnt (`InvalidArgumentException: Path element starting with
'.' is not permitted`). Der komplette Pfad zu diesem Spielordner enthält
aber `.var/app/...` (Flatpak-Steam) und `.local/share/...` — ein
Ghidra-Projekt kann deshalb an keiner Stelle innerhalb von
`re-project/ghidra/projects/` angelegt werden, egal welcher Unterordner.

## Inhalt des Projekts

Importiert (Retail-Build, `Gunman-ENG-GER/rewolf/`), vollautomatisch
analysiert (Ghidra-Standardanalyse, 0 Fehler):

- `/gunman.dll` (Server-DLL, `dlls/gunman.dll`)
- `/client.dll` (Client-DLL, `cl_dlls/client.dll`)

## Öffnen

```
/home/masterbee/Werkzeuge/ghidra_12.1.3_PUBLIC/ghidraRun
```
→ „Open Project" → `/home/masterbee/ghidra-projects/GunmanChronicles.gpr`

Für PyGhidra-Skripte (`re-project/ghidra/scripts/*.py`) auf dieses Projekt
verweisen, z. B.:

```
analyzeHeadless /home/masterbee/ghidra-projects GunmanChronicles \
    -process gunman.dll -scriptPath re-project/ghidra/scripts \
    -postScript <script>.py
```

## Nebenfund: vorhandene IDA-Pro-Datenbanken

Im selben `dlls`/`cl_dlls`-Ordner liegen bereits fertige IDA-Datenbanken
(Zeitstempel 2016), bisher nirgends dokumentiert:

- `Gunman-ENG-GER/rewolf/dlls/gunman.idb` (20 MB)
- `Gunman-ENG-GER/rewolf/cl_dlls/client.idb` (8 MB)

Deuten auf frühere, umfangreiche RE-Arbeit hin (vermutlich benannte
Funktionen/Klassen/Kommentare). `.idb` ist proprietäres IDA-Format, von
Ghidra nicht direkt lesbar — noch nicht ausgewertet, siehe STATUS.md für
den aktuellen Stand dazu.
