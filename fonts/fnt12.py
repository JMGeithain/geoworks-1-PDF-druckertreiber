"""Leser fuer GeoWorks-1.x-Schriftdateien (BSWF, Nimbus-Umrisse)."""
import struct

class Font:
    def __init__(self, path):
        d = self.d = open(path, 'rb').read()
        if d[:4] != b'BSWF':
            raise ValueError('keine GEOS-Schriftdatei')
        w = lambda o: struct.unpack_from('<H', d, o)[0]
        self.id, self.maker, self.family = w(8), w(10), d[12]
        self.name = d[13:33].split(b'\0')[0].decode('latin1')
        base = 6                                    # FontInfo beginnt mit FI_fileHandle
        ps, pe, ot, oe = w(33), w(35), w(37), w(39)
        self.outlines = []
        for o in range(base + ot, base + oe, 26):
            style, weight = d[o], d[o + 1]
            ents = [struct.unpack_from('<IHH', d, o + 2 + 8 * k)[:2] for k in range(3)]
            self.outlines.append((style, weight, ents))
        self.bitmaps = (pe - ps) // 10 if ps else 0

    def outline(self, k=0):
        return Outline(self, *self.outlines[k])

class Outline:
    def __init__(self, font, style, weight, ents):
        d = font.d
        self.style, self.weight = style, weight
        (ho, hs), (p1o, p1s), (p2o, p2s) = ents
        h = d[ho:ho + hs]
        self.h_height, self.x_height, self.ascender, self.descender = struct.unpack_from('<4h', h, 0)
        (self.avgw, self.maxw, self.height, self.accent, self.ascent, self.descent,
         self.baseAdjust) = struct.unpack_from('<7h', h, 8)
        self.first, self.last, self.default = h[22], h[23], h[24]
        self.underPos, self.underThick, self.strikePos, self.numChars = struct.unpack_from('<4h', h, 25)
        self.widths = {}
        for i in range(self.numChars):
            wd, fl = struct.unpack_from('<HB', h, 43 + 3 * i)
            self.widths[self.first + i] = (wd, fl)
        self.p1, self.p2 = d[p1o:p1o + p1s], d[p2o:p2o + p2s]

    def chardata(self, c):
        if c < self.first or c > self.last:
            return None
        if c < 0x80:
            part, idx = self.p1, c - self.first
        else:
            part, idx = self.p2, c - 0x80
        if 2 * idx + 2 > len(part):
            return None
        off = struct.unpack_from('<H', part, 2 * idx)[0]
        if off == 0 or off >= len(part):
            return None
        return part, off

    def path(self, c, dx=0, dy=0, depth=0):
        """Liste von ('m',x,y) ('l',x,y) ('c',x1,y1,x2,y2,x3,y3); Einheiten 1/1000 em, y nach oben."""
        r = self.chardata(c)
        if r is None or depth > 3:
            return []
        part, p = r
        p += 8                                       # NimbusData (Begrenzung)
        for _ in range(2):                           # x- und y-Tripel ueberspringen
            p += 1 + 6 * part[p]
        out, cx, cy = [], 0, 0
        def sw(o): return struct.unpack_from('<h', part, o)[0]
        def sb(o): return struct.unpack_from('<b', part, o)[0]
        while p < len(part):
            cmd = part[p]; p += 1
            if cmd == 0:
                cx, cy = sw(p), sw(p + 2); p += 4; out.append(('m', cx + dx, cy + dy))
            elif cmd == 1:
                cx, cy = sw(p), sw(p + 2); p += 4; out.append(('l', cx + dx, cy + dy))
            elif cmd == 2:
                v = [sw(p + 2 * k) for k in range(6)]; p += 12
                out.append(('c', v[0] + dx, v[1] + dy, v[2] + dx, v[3] + dy, v[4] + dx, v[5] + dy)); cx, cy = v[4], v[5]
            elif cmd == 3:
                break
            elif cmd == 5:
                c1, x, y, c2 = part[p], sw(p + 1), sw(p + 3), part[p + 5]; p += 6
                out += self.path(c1, dx, dy, depth + 1)
                out += self.path(c2, dx + x, dy + y, depth + 1)
                break
            elif cmd == 6:
                cy += sw(p); p += 2; out.append(('l', cx + dx, cy + dy))
            elif cmd == 7:
                cx += sw(p); p += 2; out.append(('l', cx + dx, cy + dy))
            elif cmd == 8:
                cy += sb(p); cx += sb(p + 1); p += 2; out.append(('l', cx + dx, cy + dy))
            elif cmd == 9:
                pts, x, y = [], cx, cy
                for k in range(3):
                    y += sb(p); x += sb(p + 1); p += 2; pts += [x + dx, y + dy]
                out.append(('c', *pts)); cx, cy = x, y
            else:
                raise ValueError('Befehl %d in Zeichen %d' % (cmd, c))
        return out

    def bbox(self, c):
        r = self.chardata(c)
        if r is None: return None
        part, p = r
        return struct.unpack_from('<4h', part, p)
