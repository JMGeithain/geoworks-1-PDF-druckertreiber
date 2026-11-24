#!/usr/bin/env python3
"""Fuehrt PDF.GEO in einem 8086-Emulator aus (Unicorn) und spielt einen
Druckauftrag des Spoolers nach. Kernel-Aufrufe werden durch Python-Stubs
ersetzt, die das in doc/KERNEL12.md beschriebene Verhalten nachbilden.

usage: emutest.py PDF.GEO ausgabe.pdf
"""
import struct, sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'tools'))
import g1dump as gd
import os, fnmatch
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INTR
# GeoWorks-Verzeichnis (fuer FONT\*.FNT); ohne Angabe werden keine Schriften gefunden
GEOTOP = os.environ.get('GEOWORKS_DIR', '')
FIXLEN = {0:0,2:0,4:0,5:0,6:0,7:4,8:8,9:8,10:24,11:24,12:0,13:8,14:4,15:8,16:4,17:6,18:2,19:6,20:2,23:4,24:0,
 27:6,28:8,29:5,30:1,36:8,40:8,41:4,46:8,47:1,48:4,49:4,50:3,51:1,52:1,53:2,54:1,55:1,56:10,57:4,58:2,59:3,60:1,61:1,62:6,
 63:3,64:1,65:1,66:2,67:2,68:3,69:20,70:5,72:2,73:2,74:2,75:2,76:2,77:1,78:1,79:8,80:1,81:8,82:1,83:8,85:2,86:0,87:0,88:10,89:10}
def split_gstring(b):
    out, p = [], 0
    while p < len(b):
        op = b[p]
        if op == 0: break
        if op in FIXLEN: n = 1 + FIXLEN[op]
        elif op == 1: n = 3 + struct.unpack_from('<H', b, p + 1)[0]
        elif op == 3: n = 5 + struct.unpack_from('<H', b, p + 3)[0]
        elif op in (25, 26):
            q = p + (5 if op == 25 else 1)
            while True:
                sz = struct.unpack_from('<H', b, q)[0]; bm = b[q + 2:q + 2 + 20]
                w, h, cmp_, typ, st, ns = struct.unpack_from('<HHBBHH', bm)
                q += 2 + sz
                if not (typ & 8) or st + ns >= h: break
            n = q - p
        elif op == 31: n = 7 + struct.unpack_from('<H', b, p + 5)[0]
        elif op == 32: n = 3 + struct.unpack_from('<H', b, p + 1)[0]
        elif op in (35, 38, 39): n = 3 + 4 * struct.unpack_from('<H', b, p + 1)[0]
        elif op == 45: n = 4 + 4 * struct.unpack_from('<H', b, p + 2)[0]
        else: raise ValueError('op %02x' % op)
        if 13 <= op <= 46: out.append((b[p:p + n], (1, 0, 0, 1, 0, 0)))
        p += n
    return out
from unicorn.x86_const import *

SEG = {2: 0x1000, 1: 0x2000, 3: 0x2800, 4: 0x2900}   # Ressource -> Segment
HANDLE = {3: 0x0300, 4: 0x0400}
STUB = 0xF000                                          # Kernel-Stubs F000:Ordinal*4

def lin(s, o): return (s << 4) + o

class Emu:
    def __init__(self, path):
        g = gd.parse(path)
        self.mu = Uc(UC_ARCH_X86, UC_MODE_16)
        self.mu.mem_map(0, 0x100000)
        for n, s in SEG.items():
            data = bytearray(g['res'][n]['data'])
            for info, extra, op in g['res'][n]['rels']:
                src, kind = info >> 4, info & 15
                val = struct.unpack_from('<H', data, op)[0]
                if src == 0:                 # Kernel-Far-Pointer
                    struct.pack_into('<HH', data, op, val * 4, STUB)
                elif src == 2 and kind == 2:
                    struct.pack_into('<H', data, op, SEG[val])
                elif src == 2 and kind == 3:
                    struct.pack_into('<H', data, op, HANDLE.get(val, val))
            self.mu.mem_write(lin(s, 0), bytes(data))
        self.mu.mem_write(lin(STUB, 0), b'\xcb' * 0x1000)   # RETF
        self.mu.hook_add(UC_HOOK_CODE, self.hook, begin=lin(STUB, 0), end=lin(STUB, 0xfff))
        self.mu.hook_add(UC_HOOK_INTR, self.intr)
        self.files = {}; self.dta = (0, 0); self.find = []; self.lists = {}; self.live = set()
        self.blocks = {0x0300: SEG[3], 0x0400: SEG[4]}
        self.next_seg = 0x4000
        self.out = bytearray()
        self.gstates = 0x5000
        self.elements = []
        self.calls = []
        # PState
        self.pstate = self.alloc(0x200)

    def alloc(self, size):
        h = 0x7000 + len(self.blocks) * 0x10
        self.blocks[h] = self.next_seg
        self.next_seg += (size + 15) // 16 + 1
        return h

    def cstr(self, seg, off):
        b = bytes(self.mu.mem_read(lin(seg, off), 80)); return b[:b.index(0)].decode('latin1')
    def setcf(self, on):
        f = self.r(UC_X86_REG_EFLAGS); self.mu.reg_write(UC_X86_REG_EFLAGS, (f | 1) if on else (f & ~1))
    def path(self, p): return os.path.join(GEOTOP, *p.split('\\'))
    def intr(self, mu, intno, ud):
        assert intno == 0x21, intno
        ah = self.r(UC_X86_REG_AX) >> 8
        if ah == 0x2f:
            self.w(UC_X86_REG_ES, 0x8100); self.w(UC_X86_REG_BX, 0x80)
        elif ah == 0x1a:
            self.dta = (self.r(UC_X86_REG_DS), self.r(UC_X86_REG_DX))
        elif ah in (0x4e, 0x4f):
            if ah == 0x4e:
                pat = self.cstr(self.r(UC_X86_REG_DS), self.r(UC_X86_REG_DX))
                d, m = pat.rsplit('\\', 1)
                self.find = sorted(f for f in os.listdir(self.path(d)) if fnmatch.fnmatch(f.upper(), m))
            if self.find:
                n = self.find.pop(0).upper().encode() + b'\0'
                mu.mem_write(lin(self.dta[0], self.dta[1] + 0x1e), n); self.setcf(False)
            else:
                self.w(UC_X86_REG_AX, 18); self.setcf(True)
        else:
            raise RuntimeError('INT 21h AH=%02x' % ah)
    def r(self, reg): return self.mu.reg_read(reg)
    def set_elements(self, els):
        self.elements = list(els); self.lists[0x1234] = self.elements
    def w(self, reg, v): self.mu.reg_write(reg, v & 0xffff)

    def hook(self, mu, addr, size, ud):
        ordn = (addr - lin(STUB, 0)) // 4
        self.calls.append(ordn)
        R, W = self.r, self.w
        if ordn == 0:            # MemAlloc
            h = self.alloc(R(UC_X86_REG_AX) + 0x200)
            W(UC_X86_REG_BX, h); W(UC_X86_REG_AX, self.blocks[h])
        elif ordn == 5:          # MemLock
            W(UC_X86_REG_AX, self.blocks[R(UC_X86_REG_BX)])
        elif ordn == 212:        # GrDestroyState
            di = R(UC_X86_REG_DI); assert di in self.live, 'GrDestroyState: ungueltiges Handle %x' % di; self.live.discard(di)
        elif ordn == 279:        # GrDestroyGString(SI, DI = GState oder 0)
            di = R(UC_X86_REG_DI); assert di in self.lists or di == 0x6000, 'GrDestroyGString: ungueltiges DI %x' % di
        elif ordn in (6, 2, 24):
            pass
        elif ordn == 25:         # LMemAlloc -> Chunk-Handle 0x10, Daten bei 0x20
            seg = R(UC_X86_REG_DS)
            mu.mem_write(lin(seg, 0x10), struct.pack('<H', 0x22))
            mu.mem_write(lin(seg, 0x20), struct.pack('<H', R(UC_X86_REG_CX) + 2))
            W(UC_X86_REG_AX, 0x10)
        elif ordn == 28:         # LMemReAlloc
            seg = R(UC_X86_REG_DS)
            mu.mem_write(lin(seg, 0x20), struct.pack('<H', R(UC_X86_REG_CX) + 2))
        elif ordn == 59:         # FileWrite
            ds, dx, cx = R(UC_X86_REG_DS), R(UC_X86_REG_DX), R(UC_X86_REG_CX)
            assert R(UC_X86_REG_BX) == 5, 'falsches Datei-Handle'
            self.out += mu.mem_read(lin(ds, dx), cx)
            W(UC_X86_REG_CX, cx)
        elif ordn in (73, 74, 76):
            pass
        elif ordn == 52:         # FileOpen
            p = self.path(self.cstr(R(UC_X86_REG_DS), R(UC_X86_REG_DX)))
            if os.path.exists(p):
                h = 10 + len(self.files); self.files[h] = open(p, 'rb'); W(UC_X86_REG_AX, h); self.setcf(False)
            else:
                W(UC_X86_REG_AX, 2); self.setcf(True)
        elif ordn == 54:
            self.files.pop(R(UC_X86_REG_BX)).close(); self.setcf(False)
        elif ordn == 60:         # FilePos CX:DX
            f = self.files[R(UC_X86_REG_BX)]; f.seek((R(UC_X86_REG_CX) << 16) | R(UC_X86_REG_DX))
            W(UC_X86_REG_AX, f.tell() & 0xffff); W(UC_X86_REG_DX, f.tell() >> 16)
        elif ordn == 58:         # FileRead
            f = self.files[R(UC_X86_REG_BX)]; b = f.read(R(UC_X86_REG_CX))
            mu.mem_write(lin(R(UC_X86_REG_DS), R(UC_X86_REG_DX)), b)
            self.setcf(len(b) < R(UC_X86_REG_CX)); W(UC_X86_REG_CX, len(b)); W(UC_X86_REG_AX, len(b))
        elif ordn == 211:        # GrCreateState
            self.gstates += 1; self.live.add(self.gstates); W(UC_X86_REG_DI, self.gstates)
        elif ordn == 281:        # GrCreateGString -> DI; merkt Zielblock
            self.copy_target = R(UC_X86_REG_BX); W(UC_X86_REG_DI, 0x6000)
        elif ordn == 280:        # GrLoadGString(CL=0, BX=Block, SI=Chunk) -> SI
            seg = self.blocks[R(UC_X86_REG_BX)]
            off = struct.unpack('<H', bytes(mu.mem_read(lin(seg, R(UC_X86_REG_SI)), 2)))[0]
            n = struct.unpack('<H', bytes(mu.mem_read(lin(seg, off - 2), 2)))[0] - 2
            raw = bytes(mu.mem_read(lin(seg, off), n))
            h = 0x5000 + len(self.lists); self.lists[h] = split_gstring(raw); W(UC_X86_REG_SI, h)
        elif ordn == 283:        # GrDrawGString mit BP=4800h
            assert R(UC_X86_REG_BP) == 0x4800
            self.elements = self.lists.setdefault(R(UC_X86_REG_SI), self.elements)
            if self.elements:
                op = self.elements[0][0][0]
                W(UC_X86_REG_BP, 4); W(UC_X86_REG_DX, op)
            else:
                W(UC_X86_REG_BP, self.endcode); W(UC_X86_REG_DX, 0)
        elif ordn == 204:        # GrCopyGString: Element in Chunk schreiben
            self.elements = self.lists[R(UC_X86_REG_SI)]
            el = self.elements.pop(0)[0]
            seg = self.blocks[self.copy_target]
            mu.mem_write(lin(seg, 0x20), struct.pack('<H', len(el) + 2) + bytes(el))
        elif ordn in (259, 260, 261):
            col = {259: (0, 0, 0xaa), 260: (0xaa, 0, 0), 261: (0, 0, 0)}[ordn]
            W(UC_X86_REG_AX, col[0]); W(UC_X86_REG_BX, col[1] | (col[2] << 8))
        elif ordn == 271: W(UC_X86_REG_AX, 1)
        elif ordn == 286:        # GrGetTransform -> DS:SI
            m = self.elements[0][1] if self.elements else (1, 0, 0, 1, 0, 0)
            raw = b''.join(struct.pack('<i', int(round(v * 65536))) for v in m)
            mu.mem_write(lin(R(UC_X86_REG_DS), R(UC_X86_REG_SI)), raw)
        elif ordn == 247:        # GrMapColorIndex(AH) -> AL,BL,BH
            pal = {0: (0, 0, 0), 1: (0, 0, 0xaa), 4: (0xaa, 0, 0), 15: (0xff, 0xff, 0xff)}
            r_, g_, b_ = pal.get(R(UC_X86_REG_AX) >> 8, (0x55, 0x55, 0x55))
            W(UC_X86_REG_AX, (R(UC_X86_REG_AX) & 0xff00) | r_); W(UC_X86_REG_BX, g_ | (b_ << 8))
        elif ordn == 293: W(UC_X86_REG_CX, 0x1000); W(UC_X86_REG_DX, 12)
        elif ordn in (262, 263, 264): W(UC_X86_REG_AX, (self.elements[0][2] if self.elements and len(self.elements[0]) > 2 else 57) if ordn == 263 else 25)
        elif ordn == 269: W(UC_X86_REG_AX, 0x20)
        elif ordn == 275: W(UC_X86_REG_AX, 300); W(UC_X86_REG_BX, 500)
        else:
            raise RuntimeError(f'unbekannter Kernel-Aufruf {ordn}')

    def call(self, di, **regs):
        """Ruft die Strategie (res2:0000) per FAR CALL auf."""
        mu = self.mu
        ss, sp = 0x9000, 0xfff0
        ret_seg, ret_off = 0x8000, 0x0000
        mu.mem_write(lin(ret_seg, ret_off), b'\xf4')            # HLT
        sp -= 4
        mu.mem_write(lin(ss, sp), struct.pack('<HH', ret_off, ret_seg))
        self.w(UC_X86_REG_SS, ss); self.w(UC_X86_REG_SP, sp)
        for k in ('ax', 'bx', 'cx', 'dx', 'si', 'bp', 'ds', 'es'):
            self.w(getattr(sys.modules['unicorn.x86_const'], 'UC_X86_REG_' + k.upper()), regs.get(k, 0))
        self.w(UC_X86_REG_DI, di)
        self.w(UC_X86_REG_CS, SEG[2])
        mu.emu_start(lin(SEG[2], 0), lin(ret_seg, ret_off), count=2_000_000)
        assert self.r(UC_X86_REG_SP) == 0xfff0, 'Stack nicht ausgeglichen'
        return {k: self.r(getattr(sys.modules['unicorn.x86_const'], 'UC_X86_REG_' + k.upper()))
                for k in ('ax', 'bx', 'cx', 'dx', 'si', 'di', 'bp', 'eflags')}

def el(op, fmt, *v):
    return bytes([op]) + struct.pack(fmt, *v)

def main(geo, pdf):
    e = Emu(geo)
    ps = e.pstate
    r = e.call(8, bx=ps);                         print('DRIVER_INFO dx=%04x si=%04x' % (r['dx'], r['si']))
    r = e.call(6, bx=ps, dx=0, si=0)
    r = e.call(0xa, bx=ps);                 print('DEVICE_INFO dx=%04x' % r['dx'])
    r = e.call(0xc, bx=ps, cx=4, ax=595, si=842)
    r = e.call(0x10, bx=ps);                print('AREA', r['ax'], r['si'], r['cx'], r['dx'])
    r = e.call(0xe, bx=ps, cx=5, dx=0, si=4)
    r = e.call(0x1a, bx=ps)
    tm = struct.pack('<12H', 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0)  # LANDSCAPE: os.environ
    if os.environ.get('LANDSCAPE'): tm = struct.pack('<12H', 0, 0, 0, 1, 0, 0xffff, 0, 0, 0, 595, 0, 0)          # Identitaet wie im echten Lauf
    e.mu.mem_write(lin(0x3000, 0), tm)
    I = (1, 0, 0, 1, 0, 0)
    def tf(text, x, y, runs, width=0):
        """Text-Feld 21h mit 20-Byte-Attributen; runs = [(Anzahl, Farbindex, Groesse)]"""
        saved = bytearray(28)
        struct.pack_into('<h', saved, 5, x); struct.pack_into('<h', saved, 24, y)
        struct.pack_into('<h', saved, 26, 11); struct.pack_into('<h', saved, 7, width)                 # Grundlinienabstand
        rb = b''
        for run in runs:
            cnt, col, size = run[:3]; fid = run[3] if len(run) > 3 else 0x1000; sty = run[4] if len(run) > 4 else 0
            attr = bytearray(20); attr[0] = 0; attr[1] = col
            struct.pack_into('<H', attr, 13, fid); attr[15] = 0; attr[6] = sty
            struct.pack_into('<H', attr, 16, size)
            rb += struct.pack('<H', cnt) + bytes(attr)
        first = rb[:22]
        body = bytes(saved) + text + first
        return bytes([0x21]) + struct.pack('<H', len(body)) + body + rb[22:]
    page1 = [(el(0x28, '<4h', 0, 0, 595, 842), I),                       # Seitenhintergrund
             (el(0x2e, '<4h', 0, 0, 1, 1), (150, 0, 0, 100, 180, 175)),  # GeoDraw: Einheitsellipse
             (el(0x24, '<4h', 0, 0, 1, 1), (150, 0, 0, 100, 180, 175)),
             (el(0x28, '<4h', 0, 0, 1, 1), (174, 0, 0, 208, 376, 246)),
             (el(0x0f, '<4h', 0, 0, 1, 1), (174, 0, 0, 208, 376, 246)),
             (tf(b'Text', 0, 0, [(4, 0, 14, 0x1000, 0x01)], 40), (1, 0, 0, 1, 94, 340)),
             (tf(b'\x80\x85\x86 Rot', 0, 24, [(3, 0, 14, 0x1200, 0x20), (4, 4, 18, 0x1000, 0x0a)]), (1, 0, 0, 1, 94, 340)),
             (el(0x0f, '<4h', -50, -20, 50, 20), (0.866, 0.5, -0.5, 0.866, 300, 650)),  # gedreht 30 Grad
             (bytes([0x1f]) + struct.pack('<3h', 0, 0, 7) + b'gedreht', (0.866, 0.5, -0.5, 0.866, 230, 700)),
             (el(0x26, '<h', 0), I)]
    def cbm(w, start, n, fmt, data, pal=b'', comp=0):
        typ = 8 | fmt | (0x40 if pal else 0)
        hdr = struct.pack('<HHBB7H', w, 0, comp, typ, start, n, 0, 20 + len(pal), 20 if pal else 0, 72, 72)
        hdr = hdr[:2] + struct.pack('<H', 40) + hdr[4:]
        return hdr + pal + data
    # Mono 16x40 in zwei Scheiben: Schachbrett
    row = lambda i: bytes([0xF0 if (i // 4) % 2 else 0x0F]) * 2
    s1 = cbm(16, 0, 24, 0, b''.join(row(i) for i in range(24)), pal=bytes(48))
    s2 = cbm(16, 24, 16, 0, b''.join(row(i) for i in range(24, 40)))
    bm = bytes([0x19]) + struct.pack('<hh', 0, 0) + struct.pack('<H', len(s1)) + s1 + struct.pack('<H', len(s2)) + s2
    # 4-Bit 8x4 einfach, PackBits: jede Zeile = 4 Bytes gleicher Farbe
    pk = b''.join(bytes([0xFD, (c << 4) | c]) for c in (1, 4, 2, 14))
    bm4 = bytes([0x19]) + struct.pack('<hh', 0, 0) + struct.pack('<H', 6 + len(pk)) + struct.pack('<HHBB', 8, 4, 1, 1) + pk
    page2 = [(el(0x0d, '<4h', 0, 0, 595, 842), I),
             (bm, (4, 0, 0, 4, 100, 100)),
             (bm4, (10, 0, 0, 10, 300, 100)),
             (tf(b'q Auto-Guthannss \x80\x8a', 0, 0, [(2, 0, 24, 0x1801, 0), (14, 0, 30, 0x160a, 0), (3, 4, 30, 0x160a, 0x30)], 330), (1, 0, 0, 1, 60, 400)),
             (tf(b'Fraktur Bold', 0, 0, [(12, 1, 30, 0xd042, 0x20)], 200), (1, 0, 0, 1, 60, 500)),
             (el(0x28, '<4h', 300, 300, 400, 380), I, 6),
             (el(0x28, '<4h', 420, 300, 520, 380), I, 8),
             (el(0x2e, '<4h', 300, 400, 400, 480), I, 4),
             (tf(b'R\xd9W', 0, 0, [(3, 0, 60, 0xd04c, 0)], 0), (1, 0, 0, 1, 60, 600))]
    fx = os.environ.get('TEXTFIELD_FIXTURE')  # optional: echtes Textfeld (Opcode 21h) mit eingebetteter Grafik
    if fx and os.path.exists(fx):
        page1 = page1 + [(open(fx, 'rb').read(), (1, 0, 0, 1, 0, 0))]
    for n, (elements, end) in enumerate([(page1, 1), (page2, 0)]):
        e.set_elements(elements); e.endcode = 0xff if end == 0 else 1
        e.call(0x2e, bx=ps, cx=0x0c)
        e.call(0x8100, bx=ps, dx=0x3000, si=0)
        r = e.call(0x8101, bx=ps, si=0x1234, cx=0x4000)
        print('PRINT_GSTRING Seite %d -> AX=%d, SI=%04x' % (n + 1, r['ax'], r['si']))
        assert r['si'] == 0x1234, 'SI nicht erhalten'
        e.call(0x30, bx=ps, cx=0x0c)
    e.call(0x1c, bx=ps)
    open(pdf, 'wb').write(e.out)
    print('Kernel-Aufrufe:', sorted(set(e.calls)))
    print(len(e.out), 'Bytes PDF')

if __name__ == '__main__':
    main(*sys.argv[1:3])
