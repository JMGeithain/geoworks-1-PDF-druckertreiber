#!/usr/bin/env python3
"""Disassemble one resource of a GEOS 1.x geode with relocation annotations.
usage: g1dis.py FILE RES [start] [end]"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import g1dump as d
from capstone import Cs, CS_ARCH_X86, CS_MODE_16

KNOWN = {0: 'MemAlloc', 2: 'MemFree', 5: 'MemLock', 6: 'MemUnlock', 149: 'GeodeGetProcessHandle',
         150: 'GeodeGetResourceHandle', 169: 'ObjMessage', 184: 'TimerSleep', 211: 'GrCreateState',
         212: 'GrDestroyState', 316: 'GrDrawRect', 324: 'GrDrawEllipse', 326: 'GrDrawBitmap',
         330: 'GrDrawText', 336: 'GrFillRect', 346: 'GrSetLineColor', 355: 'GrSetAreaColor',
         356: 'GrSetAreaMask', 359: 'GrSetTextColor', 362: 'GrSetTextStyle', 366: 'GrSetFont',
         440: 'GrGetWinBounds', 446: 'GrBeginUpdate', 447: 'GrEndUpdate', 489: 'ObjCallSuperNoLock',
         51: 'FileSetCurrentPath', 52: 'FileOpen', 53: 'FileCreate', 54: 'FileClose', 58: 'FileRead',
         59: 'FileWrite', 60: 'FilePos'}
TYPES = ['FARPTR', 'OFF', 'SEG', 'HANDLE', 'CALL', 'XIP']

def annotate(g, r):
    ann = {}
    for info, extra, op in r['rels']:
        src, kind = info >> 4, info & 15
        word = d.w(r['data'], op)
        if src == 0:
            nm = KNOWN.get(word, f'kern#{word}')
            # far ptr/call: operand is offset word followed by seg word
            ann[op] = f'{TYPES[kind]} {nm}'
        elif src == 1:
            ann[op] = f'{TYPES[kind]} {g["imports"][extra][0]}#{word}'
        else:
            ann[op] = f'{TYPES[kind]} res{word}'
    return ann

def dis(path, resn, start=0, end=None):
    g = d.parse(path); r = g['res'][resn]
    data = r['data']; end = end or len(data)
    ann = annotate(g, r)
    md = Cs(CS_ARCH_X86, CS_MODE_16); md.skipdata = True
    for ins in md.disasm(data[start:end], start):
        notes = [ann[o] for o in range(ins.address, ins.address + ins.size) if o in ann]
        print(f"{ins.address:04x}: {ins.bytes.hex():16s} {ins.mnemonic:6s} {ins.op_str:28s} {'; ' + ', '.join(notes) if notes else ''}")

if __name__ == '__main__':
    a = sys.argv
    dis(a[1], int(a[2]), int(a[3], 16) if len(a) > 3 else 0, int(a[4], 16) if len(a) > 4 else None)
