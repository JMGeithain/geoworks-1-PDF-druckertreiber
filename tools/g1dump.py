#!/usr/bin/env python3
"""Minimal PC/GEOS 1.x geode dumper (layout per pcgeos-1-sdk-v2 Format.pm)."""
import struct, sys, re

def w(b, o): return struct.unpack_from('<H', b, o)[0]

def parse(path):
    raw = open(path, 'rb').read()
    assert raw[:4] == b'\xc7\x45\xcf\x53', 'not GEOS1'
    g = dict(path=path, raw=raw)
    g['ftype_file'] = w(raw, 4)
    g['long'] = raw[0x20:0x44].split(b'\0')[0].decode('latin1')
    g['token'] = raw[0x14:0x18].decode('latin1')
    g['release'] = struct.unpack_from('<4H', raw, 8)
    g['protocol'] = struct.unpack_from('<2H', raw, 16)
    g['attr'] = w(raw, 0xc8); g['gtype'] = w(raw, 0xca)
    g['kproto'] = struct.unpack_from('<2H', raw, 0xcc)
    g['name'] = raw[0xf4:0x100].decode('latin1')
    g['entry'] = (w(raw, 0x10c), w(raw, 0x10e))  # offset, resource
    nres, nimp, nexp = w(raw, 0x118), w(raw, 0x114), w(raw, 0x112)
    ia = 0x120; ea = ia + 14 * nimp; ra = ea + 4 * nexp
    g['imports'] = []
    for i in range(nimp):
        p = ia + 14 * i
        g['imports'].append((raw[p:p+8].decode('latin1').rstrip(), w(raw, p+8), struct.unpack_from('<2H', raw, p+10)))
    g['exports'] = [struct.unpack_from('<2H', raw, ea + 4*i) for i in range(nexp)]
    g['res'] = []
    for i in range(nres):
        size = w(raw, ra + 2*i); off = struct.unpack_from('<I', raw, ra + 2*nres + 4*i)[0]
        rb = w(raw, ra + 6*nres + 2*i); fl = w(raw, ra + 8*nres + 2*i)
        data = raw[off:off+size]
        rel_at = off + ((size + 15) & ~15)
        rels = [struct.unpack_from('<BBH', raw, rel_at + j) for j in range(0, rb, 4)]
        g['res'].append(dict(n=i, size=size, off=off, flags=fl, data=data, rels=rels))
    return g

def strings(b, n=5):
    return [m.group().decode('latin1') for m in re.finditer(rb'[\x20-\x7e\xc0-\xff]{%d,}' % n, b)]

if __name__ == '__main__':
    for p in sys.argv[1:]:
        g = parse(p)
        print(f"=== {p}: '{g['long']}' token={g['token']} name={g['name']!r} gtype={g['gtype']} attr={g['attr']:#06x} rel={g['release']} proto={g['protocol']} kproto={g['kproto']} entry=res{g['entry'][1]}:{g['entry'][0]:#x}")
        for i, (n, a, pr) in enumerate(g['imports']):
            print(f"  import[{i}] {n:8s} attr={a:#x} proto={pr}")
        print('  exports:', ' '.join(f"{i}:r{r}:{o:#x}" for i, (o, r) in enumerate(g['exports'])))
        for r in g['res']:
            kinds = {}
            for info, extra, op in r['rels']:
                kinds.setdefault((info >> 4, info & 15, extra if info >> 4 == 1 else None), 0)
                kinds[(info >> 4, info & 15, extra if info >> 4 == 1 else None)] += 1
            print(f"  res{r['n']}: size={r['size']:5d} flags={r['flags']:#06x} rels={len(r['rels'])}")
