#!/usr/bin/env python3
import sys
import pyghidra

binary_path = sys.argv[1]
project_location = sys.argv[2]
project_name = sys.argv[3]
out_path = sys.argv[4]

pyghidra.start()

from ghidra.program.util import DefinedDataIterator  # noqa: E402

with pyghidra.open_program(
    binary_path,
    project_location=project_location,
    project_name=project_name,
    analyze=False,
    nested_project_location=False,
) as flat_api:
    program = flat_api.getCurrentProgram()
    fm = program.getFunctionManager()
    with open(out_path, "w") as f:
        for func in fm.getFunctions(True):
            addr = func.getEntryPoint()
            name = func.getName(True)  # True = include namespace/class qualifier
            sig = func.getSignature().getPrototypeString(False)
            f.write("0x%s\t%s\t%s\n" % (addr, name, sig))
print("OK: wrote", out_path)
