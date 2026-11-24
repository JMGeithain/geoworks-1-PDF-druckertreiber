#!/usr/bin/env python3
"""Baut den GeoWorks-1.2-Druckertreiber PDF.GEO.

usage: build_driver.py pdfdrv.asm ausgabe.geo [nasm]

Aufbau (wie LASERJET.GEO/PSCRIPT.GEO, Felder in doc/DRIVER12.md):
  res0  Core, leer                                  Flags 0880h
  res1  DGROUP: DriverTable + Variablen (fest)      Flags 0080h
  res2  Code (fest), aus NASM                       Flags 0AC0h
  res3  Geraete-Info (PrinterInfo + Grafikeigenschaften) Flags 0972h
  res4  Treiber-Info als LMem-Block (Namen, Tabellen)    Flags 097Ah
Der Geode-Kopf folgt dem 1.x-Format (Kopf 120h Byte). Treiberattribut 3000h,
Typ 3, Treibertabelle in Ressource 1, Offset 0 (Kopf +108h/+10Ah).
"""
import os, struct, subprocess, sys, tempfile

HEADER = 0x120
DGROUP_SIZE = None            # wird aus DG_SIZE in der .asm gelesen

def put(b, off, fmt, *v):
    struct.pack_into(fmt, b, off, *v)

def assemble(src, nasm):
    out = tempfile.mktemp(suffix='.bin')
    lst = os.path.splitext(sys.argv[2])[0] + '.lst'
    r = subprocess.run([nasm, '-f', 'bin', '-I', os.path.dirname(os.path.abspath(src)) + '/',
                        '-o', out, '-l', lst, src], capture_output=True, text=True)
    if r.returncode:
        sys.exit(r.stdout + r.stderr)
    raw = open(out, 'rb').read(); os.unlink(out)
    i = raw.rfind(b'PDFMETA1')
    code_len, rel_end = struct.unpack_from('<HH', raw, i + 8)
    rel_len = rel_end - code_len
    code = raw[:code_len]
    rels = [struct.unpack_from('<BBH', raw, code_len + k) for k in range(0, rel_len, 4)]
    for info, extra, op in rels:
        if op + 2 > len(code):
            sys.exit('Relocation ausserhalb des Codes')
    return code, rels

def device_info():
    """PrinterInfo (1.x): Typ, Anschluesse, Smarts, Modus-Zeiger, Raender, Papier, GP."""
    b = bytearray(0x20)
    b[0] = 0x02               # Typ wie Farb-PostScript-Geraet
    b[1] = 0x1f               # Anschluesse inkl. Datei
    b[2] = 0x02               # PS_PDL: Seiten kommen als GString
    put(b, 3, '<5H', 0, 0, 0x17, 0, 0)   # nur Modus "hoch" (Offset 7)
    put(b, 0x0d, '<4H', 0, 0, 0, 0)      # Raender links/oben/rechts/unten
    b[0x15] = 0x11; b[0x16] = 0x11       # Papierzufuhr/-klasse wie PSCRIPT
    put(b, 0x17, '<HHHBB', 300, 300, 1, 1, 3)   # GraphicsProperties
    return bytes(b)

def lmem_driverinfo(resid, devinfo_resid, names, timeout=90):
    """LMem-Block wie LASERJET.GEO Res. 6. Liefert (Daten, Relocations)."""
    nnames = len(names)
    # Feste Struktur nach dem 16-Byte-Kopf
    ext = 0x10
    name_tab = ext + 12                       # nach DEIT(6) + PDI(6)
    info_tab = name_tab + 2 * nnames + 2      # Namensliste mit 0 abgeschlossen
    after = info_tab + 2 * nnames
    handles_at = (after + 3) & ~3
    chunks = [n.encode('cp437') + b'\0' for n in names] + [b';;\0']
    nh = len(chunks) + 1                      # +1 freies Handle wie im Original
    data_at = handles_at + 2 * nh
    blob = bytearray(data_at + 4 + 64 * len(chunks))
    handle_offsets = []
    pos = data_at
    for c in chunks:
        pos = (pos + 3) & ~3                   # Laengenwort auf 4 Byte, Daten bei 4n+2
        put(blob, pos, '<H', len(c) + 2)       # Laengenwort zaehlt sich mit
        blob[pos + 2:pos + 2 + len(c)] = c
        handle_offsets.append(pos + 2)
        pos += 2 + len(c)
    end = (pos + 3) & ~3
    blob = blob[:end]
    hnd = [handles_at + 2 * i for i in range(nh)]
    put(blob, 0, '<8H', resid, handles_at, 0, 0, end, nh, 0, 0)
    put(blob, ext, '<3H', nnames, name_tab, info_tab)
    put(blob, ext + 6, '<3H', timeout, 0, hnd[nnames])   # PDI: Timeout, Flags, ASCII-Tabelle
    for i in range(nnames):
        put(blob, name_tab + 2 * i, '<H', hnd[i])
        put(blob, info_tab + 2 * i, '<H', devinfo_resid)
    for i, off in enumerate(handle_offsets):
        put(blob, handles_at + 2 * i, '<H', off)
    rels = [(0x23, 0, 0)] + [(0x23, 0, info_tab + 2 * i) for i in range(nnames)]
    return bytes(blob), rels

def geode(resources, name, ext, long_name, token, release, protocol):
    nr = len(resources)
    h = bytearray(HEADER)
    h[0:4] = b'\xc7\x45\xcf\x53'
    put(h, 8, '<4H', *release); put(h, 16, '<2H', *protocol)
    h[0x14:0x18] = token; h[0x1a:0x1e] = b'GEOS'
    ln = long_name.encode('cp437')[:35]; h[0x20:0x20 + len(ln)] = ln
    cr = b'(C) 2026 GW1PDF, MIT'; h[0xa8:0xa8 + len(cr)] = cr
    for a, b2, v in ((0xc8, 0xe2, 0x3000), (0xca, 0xe4, 3), (0xd0, 0x118, nr), (0xd2, 0x114, 0), (0xd4, 0x112, 0)):
        put(h, a, '<H', v); put(h, b2, '<H', v)
    put(h, 0xcc, '<2H', 622, 3)
    h[0xe6:0xee] = h[8:16]; h[0xee:0xf2] = h[16:20]
    h[0xf4:0xfc] = name.ljust(8).encode()[:8]; h[0xfc:0x100] = ext.ljust(4).encode()[:4]
    h[0x100:0x104] = token
    put(h, 0x108, '<2H', 0, 1)                # Treibertabelle: Res. 1, Offset 0
    tables = bytearray(nr * 10)
    payload = bytearray()
    base = HEADER + len(tables)
    for i, (data, flags, rels) in enumerate(resources):
        recs = b''.join(struct.pack('<BBH', inf, ex, off) for inf, ex, off in rels)
        put(tables, 2 * i, '<H', len(data))
        put(tables, 2 * nr + 4 * i, '<I', base + len(payload) if (data or recs) else base + len(payload))
        put(tables, 6 * nr + 2 * i, '<H', len(recs))
        put(tables, 8 * nr + 2 * i, '<H', flags)
        payload += data + b'\0' * ((16 - len(data) % 16) % 16) + recs
    return bytes(h) + bytes(tables) + bytes(payload)

def main(src, out, nasm='nasm'):
    global DGROUP_SIZE
    import re
    DGROUP_SIZE = int(re.search(r'^DG_SIZE\s+equ\s+(0x[0-9a-fA-F]+|\d+)', open(src).read(), re.M).group(1), 0)
    code, coderels = assemble(src, nasm)
    dg = bytearray(DGROUP_SIZE)
    put(dg, 0, '<5H', 0, 2, 0x2000, 9, 4)     # Strategie res2:0000, Attr, Typ Drucker, Info-Block res4
    dgrels = [(0x22, 0, 2), (0x23, 0, 8)]
    devinfo = device_info()
    info, inforels = lmem_driverinfo(4, 3, ['PDF-Datei (In Datei drucken)'])
    res = [(b'', 0x0880, []), (bytes(dg), 0x0080, dgrels), (code, 0x0ac0, coderels),
           (devinfo, 0x0972, []), (info, 0x097a, inforels)]
    raw = geode(res, 'pdf', 'drvr', 'PDF-Druckertreiber', b'PRDR', (0, 1, 0, 1), (1, 0))
    open(out, 'wb').write(raw)
    print(f'{out}: {len(raw)} Bytes, Code {len(code)} Bytes, {len(coderels)} Relocations')

if __name__ == '__main__':
    main(*sys.argv[1:4])
