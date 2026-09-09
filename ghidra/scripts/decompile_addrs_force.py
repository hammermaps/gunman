#!/usr/bin/env python3
"""Wie decompile_addrs.py, aber erzwingt vorher CreateFunctionCmd an jeder
Adresse, falls Ghidra dort noch keine Funktion erkannt hat (behebt Faelle,
in denen die automatische Analyse Funktionsgrenzen verpasst hat)."""
import sys
import pyghidra

binary_path = sys.argv[1]
project_location = sys.argv[2]
project_name = sys.argv[3]
addrs = sys.argv[4:-1]
out_path = sys.argv[-1]

pyghidra.start()

from ghidra.app.decompiler import DecompInterface  # noqa: E402
from ghidra.app.cmd.function import CreateFunctionCmd  # noqa: E402
from ghidra.util.task import ConsoleTaskMonitor  # noqa: E402

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

    decomp = DecompInterface()
    decomp.openProgram(program)
    monitor = ConsoleTaskMonitor()

    with open(out_path, "w") as out:
        for addr_str in addrs:
            addr = addr_factory.getAddress(addr_str)
            func = fm.getFunctionAt(addr)
            if func is None:
                cmd = CreateFunctionCmd(addr)
                ok = cmd.applyTo(program)
                func = fm.getFunctionAt(addr)
                if func is None:
                    out.write("=" * 70 + "\n")
                    out.write("FUNC @ %s -> CreateFunctionCmd fehlgeschlagen (ok=%s)\n\n" % (addr_str, ok))
                    continue
            out.write("=" * 70 + "\n")
            out.write("FUNC @ %s (%s)\n" % (addr_str, func.getName(True)))
            res = decomp.decompileFunction(func, 30, monitor)
            if res.decompileCompleted():
                out.write(res.getDecompiledFunction().getC())
            else:
                out.write("decompile fehlgeschlagen: %s\n" % res.getErrorMessage())
            out.write("\n")
    decomp.dispose()
print("OK: wrote", out_path)
