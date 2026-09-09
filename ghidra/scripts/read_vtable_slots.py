#!/usr/bin/env python3
import sys
import pyghidra

binary_path = sys.argv[1]
project_location = sys.argv[2]
project_name = sys.argv[3]
vt_list_path = sys.argv[4]  # Zeilen: "0xADDR label"
n_slots = int(sys.argv[5])
out_path = sys.argv[6]

with open(vt_list_path) as f:
    targets = [line.split() for line in f if line.strip()]

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
    mem = program.getMemory()
    addr_factory = program.getAddressFactory()

    with open(out_path, "w") as out:
        for vt_str, label in targets:
            vt_addr = addr_factory.getAddress(vt_str)
            out.write("VTABLE %s (%s):\n" % (vt_str, label))
            for i in range(n_slots):
                try:
                    slot_addr = vt_addr.add(i * 4)
                    ptr_val = mem.getInt(slot_addr) & 0xFFFFFFFF
                    fn_addr = addr_factory.getAddress("0x%08x" % ptr_val)
                    f = fm.getFunctionAt(fn_addr)
                    fname = f.getName(True) if f else "FUN_%08x" % ptr_val
                    out.write("  [%d] 0x%08x %s\n" % (i, ptr_val, fname))
                except Exception as e:
                    out.write("  [%d] ERROR %s\n" % (i, e))
            out.write("\n")

print("OK: wrote", out_path)
