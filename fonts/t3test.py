import sys, glob
from fnt12 import Font
def glyph(o, c):
    wd = o.widths.get(c, (0, 0))[0]
    ops = []
    for e in o.path(c):
        if e[0] == 'm': ops.append('h %d %d m' % e[1:] if ops else '%d %d m' % e[1:])
        elif e[0] == 'l': ops.append('%d %d l' % e[1:])
        else: ops.append('%d %d %d %d %d %d c' % e[1:])
    bb = o.bbox(c) or (0, 0, 0, 0)
    return ('%d 0 %d %d %d %d d1\n' % (wd, *bb)) + ' '.join(ops) + (' h f\n' if ops else '\n')
fonts = sys.argv[2:]
objs = []
def add(s): objs.append(s); return len(objs)
res = []
for fi, fp in enumerate(fonts):
    f = Font(fp); o = f.outline()
    procs = {}
    for c in range(32, 256):
        if o.chardata(c) is None: continue
        g = glyph(o, c).encode('latin1')
        procs[c] = add(b'<< /Length %d >>\nstream\n' % len(g) + g + b'\nendstream')
    diffs = ' '.join('%d /g%d' % (c, c) for c in procs)
    cp = ' '.join('/g%d %d 0 R' % (c, n) for c, n in procs.items())
    wid = ' '.join(str(o.widths.get(c, (0, 0))[0]) for c in range(32, 256))
    fo = add(('<< /Type /Font /Subtype /Type3 /FontBBox [-200 -300 1200 1100] /FontMatrix [0.001 0 0 0.001 0 0] /CharProcs << %s >> /Encoding << /Type /Encoding /Differences [%s] >> /FirstChar 32 /LastChar 255 /Widths [%s] /Resources << >> >>' % (cp, diffs, wid)).encode())
    res.append((f.name, fo))
text = b''
y = 800
for i, (n, fo) in enumerate(res):
    s = 'Auto-Guthannss \x80\x85\x86 \x8a\x9a\x9f\xa7 0123'.encode('latin1')
    text += b'BT /T%d 22 Tf 30 %d Td (%s) Tj ET\n' % (i, y, s)
    text += b'BT /F1 8 Tf 30 %d Td (%s) Tj ET\n' % (y - 10, n.encode('latin1'))
    y -= 40
    if y < 40: break
cont = add(b'<< /Length %d >>\nstream\n' % len(text) + text + b'\nendstream')
fd = ' '.join('/T%d %d 0 R' % (i, fo) for i, (n, fo) in enumerate(res))
page = add(('<< /Type /Page /Parent %d 0 R /MediaBox [0 0 595 842] /Contents %d 0 R /Resources << /Font << %s /F1 << /Type /Font /Subtype /Type1 /BaseFont /Helvetica >> >> >> >>' % (len(objs) + 2, cont, fd)).encode())
cat = add(b'<< /Type /Catalog /Pages %d 0 R >>' % (len(objs) + 2))
pgs = add(b'<< /Type /Pages /Kids [%d 0 R] /Count 1 >>' % page)
out = b'%PDF-1.4\n'; offs = []
for i, s in enumerate(objs):
    offs.append(len(out)); out += b'%d 0 obj\n' % (i + 1) + s + b'\nendobj\n'
x = len(out)
out += b'xref\n0 %d\n0000000000 65535 f \n' % (len(objs) + 1) + b''.join(b'%010d 00000 n \n' % o for o in offs)
out += b'trailer\n<< /Size %d /Root %d 0 R >>\nstartxref\n%d\n%%%%EOF\n' % (len(objs) + 1, cat, x)
open(sys.argv[1], 'wb').write(out)
