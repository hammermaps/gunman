import sys
import pyghidra
bin_path, proj_loc, proj_name, addr_hex, count, out_path = sys.argv[1:7]
base = int(addr_hex, 0)
count = int(count)
pyghidra.start()
with pyghidra.open_program(bin_path, project_location=proj_loc, project_name=proj_name, analyze=False, nested_project_location=False) as flat:
    prog = flat.getCurrentProgram()
    mem = prog.getMemory()
    fm = prog.getFunctionManager()
    af = prog.getAddressFactory()
    with open(out_path, "w") as f:
        for i in range(count):
            a = af.getAddress(hex(base + i*4))
            try:
                val = mem.getInt(a) & 0xffffffff
            except Exception as e:
                f.write(f"{a}: ERROR {e}\n")
                continue
            target = af.getAddress(hex(val))
            func = fm.getFunctionAt(target)
            fname = func.getName() if func else ""
            f.write(f"{a}: 0x{val:08x} {fname}\n")
