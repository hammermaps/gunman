#!/usr/bin/env python3
"""
Fuer jede LINK_ENTITY_TO_CLASS-Stub-Funktion (z.B. "weapon_dml"):
1. Decompiliert den Stub.
2. Extrahiert die Vtable-Adresse (Muster "&PTR_FUN_XXXXXXXX" bzw.
   "&PTR__XXXXXXXX" je nach Ghidra-Benennung) aus dem C-Code.
3. Liest die ersten N Zeiger in dieser Vtable aus dem Speicherabbild.
4. Loest jeden Zeiger als Funktion auf und gibt deren (bereits demangelten)
   Namen aus, falls vorhanden -> verraet die tatsaechliche C++-Klasse hinter
   dem Entity-Klassennamen.
"""
import re
import sys
import pyghidra

binary_path = sys.argv[1]
project_location = sys.argv[2]
project_name = sys.argv[3]
addr_list_path = sys.argv[4]
out_path = sys.argv[5]

with open(addr_list_path) as f:
    targets = [line.split() for line in f if line.strip()]

pyghidra.start()

from ghidra.app.decompiler import DecompInterface  # noqa: E402
from ghidra.util.task import ConsoleTaskMonitor  # noqa: E402

VT_RE = re.compile(r"&(?:PTR_)?[A-Za-z0-9_]*?(\w*_)?([0-9a-fA-F]{6,8})\b")

with pyghidra.open_program(
    binary_path,
    project_location=project_location,
    project_name=project_name,
    analyze=False,
    nested_project_location=False,
) as flat_api:
    program = flat_api.getCurrentProgram()
    fm = program.getFunctionManager()
    mem = program.getMemory()
    addr_factory = program.getAddressFactory()

    decomp = DecompInterface()
    decomp.openProgram(program)
    monitor = ConsoleTaskMonitor()

    def func_name_at(addr):
        f = fm.getFunctionAt(addr)
        if f is not None:
            return f.getName(True)
        return None

    with open(out_path, "w") as out:
        for addr_str, name in targets:
            addr = addr_factory.getAddress(addr_str)
            func = fm.getFunctionAt(addr)
            out.write("ENTITY %-30s @ %s -> " % (name, addr_str))
            if func is None:
                out.write("KEINE FUNKTION\n")
                continue
            res = decomp.decompileFunction(func, 30, monitor)
            if not res.decompileCompleted():
                out.write("DECOMPILE FEHLGESCHLAGEN\n")
                continue
            c_code = res.getDecompiledFunction().getC()

            # KORREKTUR (Session 11): Bei Klassen mit zweistufiger Konstruktion
            # (Basisklasse -> abgeleitete Klasse) gibt es MEHRERE direkte
            # Vtable-Zuweisungen "*varN = &X;" im selben Konstruktor. Nur die
            # LETZTE ist die tatsaechliche, finale Vtable der Klasse. Deshalb:
            # zuerst gezielt nach "*irgendwas = &SYMBOL;" suchen (echte
            # Zeiger-Zuweisung, kein Funktionsargument wie "&LAB_xxx" in einem
            # Callback-Parameter) und die LETZTE dieser Zuweisungen nehmen.
            direct_assigns = re.findall(r"\*\s*\w+\s*=\s*&([A-Za-z_][A-Za-z0-9_]*)\s*;", c_code)
            if direct_assigns:
                candidates = [direct_assigns[-1]]  # nur die letzte/finale Zuweisung
            else:
                # Fallback (z.B. Konstruktor-Indirektion): alle "&symbol"-Referenzen
                candidates = re.findall(r"&([A-Za-z_][A-Za-z0-9_]*)", c_code)
            resolved = False
            fallback = None
            for sym in candidates:
                m = re.search(r"([0-9a-fA-F]{6,8})$", sym)
                if not m:
                    continue
                vt_addr_str = "0x" + m.group(1)
                try:
                    vt_addr = addr_factory.getAddress(vt_addr_str)
                except Exception:
                    continue
                # Erste 6 Zeiger der moeglichen Vtable auslesen (Adresse ODER Name)
                slots = []
                names_found = []
                for i in range(6):
                    try:
                        slot_addr = vt_addr.add(i * 4)
                        ptr_val = mem.getInt(slot_addr) & 0xFFFFFFFF
                        fn_addr = addr_factory.getAddress("0x%08x" % ptr_val)
                        fname = func_name_at(fn_addr)
                        slots.append(fname if fname else "FUN_%08x" % ptr_val)
                        if fname:
                            names_found.append(fname)
                    except Exception:
                        continue
                if not slots:
                    continue
                if fallback is None:
                    fallback = (vt_addr_str, slots)
                if names_found:
                    out.write("VTABLE @ %s -> %s\n" % (vt_addr_str, ", ".join(names_found[:6])))
                    resolved = True
                    break
            if not resolved:
                if fallback is not None:
                    vt_addr_str, slots = fallback
                    out.write("VTABLE @ %s -> %s  [nur unbenannte Slots]\n" % (vt_addr_str, ", ".join(slots)))
                else:
                    out.write("(keine Vtable-Kandidaten gefunden)\n")

    decomp.dispose()

print("OK: wrote", out_path)
