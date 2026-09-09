#!/usr/bin/env python3
"""Findet alle Xrefs (Code-Referenzen) auf eine gegebene Adresse und gibt
die enthaltende Funktion aus."""
import sys
import pyghidra

binary_path = sys.argv[1]
project_location = sys.argv[2]
project_name = sys.argv[3]
target_addr = sys.argv[4]
out_path = sys.argv[5]

pyghidra.start()

with pyghidra.open_program(
    binary_path,
    project_location=project_location,
    project_name=project_name,
    analyze=False,
    nested_project_location=False,
) as flat_api:
    program = flat_api.getCurrentProgram()
    fm = program.getFunctionManager()
    addr_factory = program.getAddressFactory()
    ref_mgr = program.getReferenceManager()

    addr = addr_factory.getAddress(target_addr)
    refs = ref_mgr.getReferencesTo(addr)
    with open(out_path, "w") as out:
        for ref in refs:
            from_addr = ref.getFromAddress()
            func = fm.getFunctionContaining(from_addr)
            fname = func.getName(True) if func else "???"
            out.write("XREF from %s (in %s)\n" % (from_addr, fname))
print("OK: wrote", out_path)
