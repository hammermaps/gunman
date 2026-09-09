#!/usr/bin/env python3
"""Dekodiert eine GoldSrc-SDK-TYPEDESCRIPTION-Tabelle (Save/Restore-
Datensatz) ab einer gegebenen Adresse. Struct-Layout (32-bit):
  int   fieldType;      // FIELDTYPE-Enum
  char* fieldName;      // Zeiger auf C-String
  int   fieldOffset;
  short fieldSize;
  short flags;
= 16 Byte pro Eintrag."""
import sys
import pyghidra

binary_path = sys.argv[1]
project_location = sys.argv[2]
project_name = sys.argv[3]
base_addr_str = sys.argv[4]
n_entries = int(sys.argv[5])
out_path = sys.argv[6]

FIELDTYPE_NAMES = [
    "FIELD_FLOAT", "FIELD_STRING", "FIELD_ENTITY", "FIELD_CLASSPTR",
    "FIELD_EHANDLE", "FIELD_ENTVARS", "FIELD_EDICT", "FIELD_VECTOR",
    "FIELD_POSITION_VECTOR", "FIELD_POINTER", "FIELD_INTEGER",
    "FIELD_FUNCTION", "FIELD_BOOLEAN", "FIELD_SHORT", "FIELD_CHARACTER",
    "FIELD_TIME", "FIELD_MODELNAME", "FIELD_SOUNDNAME", "FIELD_TYPECOUNT",
]

pyghidra.start()

with pyghidra.open_program(
    binary_path,
    project_location=project_location,
    project_name=project_name,
    analyze=False,
    nested_project_location=False,
) as flat_api:
    program = flat_api.getCurrentProgram()
    mem = program.getMemory()
    addr_factory = program.getAddressFactory()

    base = addr_factory.getAddress(base_addr_str)
    with open(out_path, "w") as out:
        for i in range(n_entries):
            entry_addr = base.add(i * 16)
            try:
                field_type = mem.getInt(entry_addr) & 0xFFFFFFFF
                name_ptr = mem.getInt(entry_addr.add(4)) & 0xFFFFFFFF
                field_offset = mem.getInt(entry_addr.add(8))
                field_size = mem.getShort(entry_addr.add(12)) & 0xFFFF
                flags = mem.getShort(entry_addr.add(14)) & 0xFFFF

                name_str = "?"
                if name_ptr != 0:
                    try:
                        name_addr = addr_factory.getAddress("0x%08x" % name_ptr)
                        b = bytearray()
                        a = name_addr
                        for _ in range(64):
                            c = mem.getByte(a) & 0xFF
                            if c == 0:
                                break
                            b.append(c)
                            a = a.add(1)
                        name_str = b.decode("ascii", errors="replace")
                    except Exception as e:
                        name_str = "ERR(%s)" % e

                type_name = FIELDTYPE_NAMES[field_type] if 0 <= field_type < len(FIELDTYPE_NAMES) else "UNKNOWN(%d)" % field_type

                out.write("[%2d] type=%-22s name=%-24s offset=0x%03x (%d) size=%d flags=%d\n" % (
                    i, type_name, name_str, field_offset, field_offset, field_size, flags))
            except Exception as e:
                out.write("[%2d] ERROR %s\n" % (i, e))

print("OK: wrote", out_path)
