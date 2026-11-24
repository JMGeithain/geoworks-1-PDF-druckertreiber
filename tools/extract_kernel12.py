#!/usr/bin/env python3
"""Zieht aus einer GeoWorks-1.2-Installation die Rohdaten fuer die Kernel-Namensliste.

usage: extract_kernel12.py GEOWORKS-Verzeichnis convertGStringTables.asm

1. Einsprungtabelle in KERNEL.EXE: 577 Woerter (Near-Offsets im Kernel-Code-
   segment), paragraph-ausgerichtet. Der Lader liest sie ueber cs:[2147h]
   (Relocation-Code bei Image-Offset 2149h).
2. KLIB.GEO Ressource 17: 90 Offset- und 90 Segment-Relocations auf Kernel-
   Eintraege = Abspieltabelle fuer die 90 GString-Opcodes. Zusammen mit der
   Opcode-Liste aus FreeGEOS ergibt das Namen fuer die Kernel-Eintraege.
3. Kernel-Stubs der Form "mov ax,N / call|jmp 1F1Dh|1F12h" springen in die
   KLIB-Exportnummer N.
Ausgabe: JSON auf stdout.
"""
import json, os, re, struct, sys

def w(b, o): return struct.unpack_from('<H', b, o)[0]

def kernel_table(path):
    raw = open(path, 'rb').read()
    hdr = w(raw, 8) * 16
    img = raw[hdr:]
    best = (0, 0)
    for s in range(0, len(img) - 1200, 16):
        n = 0
        while n < 700 and s + 2 * n + 2 <= len(img) and 0x10 < w(img, s + 2 * n) < 0xe000:
            n += 1
        if n > best[0]:
            best = (n, s)
    n, s = best
    return img, s, list(struct.unpack_from(f'<{n}H', img, s))

def geode(path):
    raw = open(path, 'rb').read()
    nres, nimp, nexp = w(raw, 0x118), w(raw, 0x114), w(raw, 0x112)
    ra = 0x120 + 14 * nimp + 4 * nexp
    res = []
    for i in range(nres):
        size = w(raw, ra + 2 * i); off = struct.unpack_from('<I', raw, ra + 2 * nres + 4 * i)[0]
        rb = w(raw, ra + 6 * nres + 2 * i)
        rel_at = off + ((size + 15) & ~15)
        rels = [struct.unpack_from('<BBH', raw, rel_at + j) for j in range(0, rb, 4)]
        res.append((raw[off:off + size], rels))
    return res

def klib_stub(img, a):
    """Erkennt 'mov ax,N ; call/jmp 1F1Dh/1F12h' in den ersten 24 Bytes."""
    code = img[a:a + 24]
    m = re.search(rb'\xb8(..)(\xe8|\xe9)(..)', code, re.S)
    if not m:
        return None
    n = w(m.group(1), 0)
    rel = struct.unpack('<h', m.group(3))[0]
    target = a + m.end() + rel
    return n if target in (0x1f1d, 0x1f12) else None

def main(gw, asm):
    img, base, tab = kernel_table(os.path.join(gw, 'KERNEL.EXE'))
    L = open(asm, encoding='latin1').read()
    pe = L.split('PlayElemTable')[1].split('KernRoutTable')[0]
    kr = L.split('KernRoutTable')[1].split('combineTable')[0]
    opnames = [n for _, n in re.findall(r'dw\s+offset\s+(\w+)\s*;\s*(\w+)', pe)]
    routines = [k for k, _ in re.findall(r'fptr\s+(\w+)\s*;\s*(\w+)', kr)]
    data, rels = geode(os.path.join(gw, 'SYSTEM', 'KLIB.GEO'))[17]
    offs = sorted((op, w(data, op)) for info, e, op in rels if info == 0x01)
    optable = [{'opcode': i, 'opname': opnames[i], 'kernel': ent, 'convert_routine': routines[i]}
               for i, (_, ent) in enumerate(offs)]
    stubs = {e: klib_stub(img, a) for e, a in enumerate(tab)}
    json.dump({'kernel_table_image_offset': base, 'kernel_entries': len(tab),
               'entry_offsets': tab, 'klib_stubs': {k: v for k, v in stubs.items() if v is not None},
               'gstring_optable': optable}, sys.stdout, indent=1)

if __name__ == '__main__':
    main(*sys.argv[1:3])
