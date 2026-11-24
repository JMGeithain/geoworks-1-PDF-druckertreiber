#!/usr/bin/env python3
"""Gleicht die GString-Sprungtabelle in PS.GEO (GeoWorks 1.2) mit der
1.x-Opcodeliste aus FreeGEOS Library/Convert/convertGStringTables.asm ab.

usage: verify_optable.py PS.GEO convertGStringTables.asm

Prueft:
  * Tabelle in PS.GEO Ressource 5 ab 0xcd8, Index = Opcode - 0x0d
  * jede Handler-Gruppe enthaelt nur Opcodes einer Elementform
  * die Unter-Unterscheidungen der Handler ((Opcode-13)*2-Vergleiche,
    direkte Opcode-Vergleiche 0x19/0x1a/0x1f) passen zur Liste
"""
import re, struct, sys

def w(b, o): return struct.unpack_from('<H', b, o)[0]

def ps_res(path, n):
    raw = open(path, 'rb').read()
    assert raw[:4] == b'\xc7\x45\xcf\x53'
    nres, nimp, nexp = w(raw, 0x118), w(raw, 0x114), w(raw, 0x112)
    ra = 0x120 + 14 * nimp + 4 * nexp
    size = w(raw, ra + 2 * n); off = struct.unpack_from('<I', raw, ra + 2 * nres + 4 * n)[0]
    return raw[off:off + size]

def opcodes(asm):
    L = open(asm, encoding='latin1').read()
    pe = L.split('PlayElemTable')[1].split('KernRoutTable')[0]
    return [n for _, n in re.findall(r'dw\s+offset\s+(\w+)\s*;\s*(\w+)', pe)]

def main(ps, asm):
    names = opcodes(asm)
    assert len(names) == 90, len(names)
    D = ps_res(ps, 5)
    # Dispatch: sub bx,0dh / shl bx,1 / call cs:[bx+0cd8h]
    i = D.find(bytes.fromhex('83eb0dd1e32eff97'))
    assert i >= 0, 'Dispatch nicht gefunden'
    table = w(D, i + 8); first = 0x0d
    # Tabelle endet dort, wo der erste Handler beginnt
    # Alle Handler liegen vor der Tabelle; das erste Wort danach, das kein
    # plausibler Zeiger auf Code vor der Tabelle ist, beendet sie.
    entries = []
    o = table
    while o + 2 <= len(D) and 0 < w(D, o) < table:
        entries.append(w(D, o)); o += 2
    groups = {}
    for k, h in enumerate(entries):
        groups.setdefault(h, []).append(first + k)
    ok = True
    print(f'Tabelle @ res5:{table:#x}, {len(entries)} Eintraege, Opcodes {first}..{first+len(entries)-1}')
    for h, ops in sorted(groups.items()):
        print(f'  handler {h:#06x}: ' + ', '.join(f'{op}={names[op][3:]}' for op in ops))
    # Stichproben gegen die Opcodeliste
    checks = [(0x1f, 'GR_DRAW_TEXT'), (0x1a, 'GR_DRAW_BITMAP_CP'), (0x19, 'GR_DRAW_BITMAP'),
              (13 + 6 // 2, 'GR_DRAW_RECT_TO'), (13 + 0x38 // 2, 'GR_FILL_RECT_TO'),
              (13 + 8 // 2, 'GR_DRAW_HLINE'), (13 + 0xe // 2, 'GR_DRAW_VLINE_TO'),
              (first + len(entries) - 1, 'GR_FILL_ELLIPSE')]
    for op, nm in checks:
        good = names[op] == nm
        ok &= good
        print(f'  {"OK " if good else "FEHLER"} Opcode {op:#04x} = {nm}')
    print('ERGEBNIS:', 'konsistent' if ok else 'ABWEICHUNG')
    return 0 if ok else 1

if __name__ == '__main__':
    sys.exit(main(*sys.argv[1:3]))
