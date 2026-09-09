# Ghidra headless post-script (Jython 2.7).
# Findet fuer jede bekannte Gunman-Entity-Klassennamen-Zeichenkette alle
# Referenzen im Binary und listet die umschliessende Funktion (i.d.R. die
# LINK_ENTITY_TO_CLASS-Registrierung, aus der sich Konstruktor/Klasse ableiten
# laesst). Ausgabe als einfache Textzeilen auf stdout, vom Aufrufer in eine
# .md/.txt-Datei umgeleitet.
#
# Aufruf (Beispiel siehe run_entity_xrefs.sh):
#   analyzeHeadless <projdir> <projname> -process <program> -noanalysis \
#     -scriptPath <this dir> -postScript DumpEntityXrefs.py <classname_list_file>

from ghidra.program.model.symbol import RefType
from ghidra.util.task import ConsoleTaskMonitor

import os

args = getScriptArgs()
if len(args) < 1:
    print("USAGE: DumpEntityXrefs.py <classname_list_file>")
    exit(1)

classname_file = args[0]
with open(classname_file, "r") as f:
    wanted = set([line.strip() for line in f if line.strip()])

listing = currentProgram.getListing()
mem = currentProgram.getMemory()
fm = currentProgram.getFunctionManager()
refmgr = currentProgram.getReferenceManager()

data_iter = listing.getDefinedData(True)
found_count = 0

print("=== Entity-Klassennamen -> referenzierende Funktionen (gunman.dll) ===")

for data in data_iter:
    try:
        val = data.getValue()
    except:
        continue
    if val is None:
        continue
    sval = str(val)
    if sval not in wanted:
        continue

    addr = data.getAddress()
    refs = refmgr.getReferencesTo(addr)
    ref_funcs = set()
    for ref in refs:
        from_addr = ref.getFromAddress()
        func = fm.getFunctionContaining(from_addr)
        if func is not None:
            ref_funcs.add("%s @ %s" % (func.getName(), func.getEntryPoint()))
        else:
            ref_funcs.add("(keine Funktion) @ %s" % from_addr)

    found_count += 1
    print("STRING: %-40s @ %s" % (sval, addr))
    if ref_funcs:
        for rf in sorted(ref_funcs):
            print("    -> %s" % rf)
    else:
        print("    -> (keine Referenzen gefunden)")

print("=== Ende: %d von %d gesuchten Strings im Binary gefunden ===" % (found_count, len(wanted)))
