# Erzeugt widths.inc: Zeichenbreiten (1/1000 em, /4) der PDF-Standardschriften
# fuer den GEOS-Zeichensatz (Mac-Roman-kompatibel), Zeichen 32..255.
import reportlab.pdfbase._fontdata as fd
from fontTools.agl import UV2AGL
fonts = ['Times-Roman', 'Times-Bold', 'Times-Italic', 'Times-BoldItalic', 'Helvetica', 'Helvetica-Bold']
out = ['; erzeugt von gen_widths.py - nicht von Hand aendern', 'wt_off: dw ' + ','.join(str(i * 224) for i in range(8)), 'wtab:']
for f in fonts:
    w = fd.widthsByFontGlyph[f]
    row = []
    for c in range(32, 256):
        try:
            g = UV2AGL.get(ord(bytes([c]).decode('mac_roman')))
        except Exception:
            g = None
        v = w.get(g, w['space'] if c == 0xCA else 500)
        row.append(min(255, (v + 2) // 4))
    out.append('; ' + f)
    for i in range(0, 224, 16):
        out.append(' db ' + ','.join(map(str, row[i:i + 16])))
open('widths.inc', 'w').write('\n'.join(out) + '\n')

for f, enc in (('Symbol', 'SymbolEncoding'), ('ZapfDingbats', 'ZapfDingbatsEncoding')):
    w = fd.widthsByFontGlyph[f]; E = fd.encodings[enc]
    row = [min(255, (w.get(E[c], 0) + 2) // 4) if E[c] else 0 for c in range(32, 256)]
    out.append('; ' + f)
    for i in range(0, 224, 16):
        out.append(' db ' + ','.join(map(str, row[i:i + 16])))
open('widths.inc', 'w').write('\n'.join(out) + '\n')
