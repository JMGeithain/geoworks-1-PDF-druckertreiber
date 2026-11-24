#!/usr/bin/env python3
"""Aendert in SPOOL.GEO (GeoWorks 1.2x) die feste Endung des Dialogs
"In Datei Drucken" von .PS auf .PDF (oder zurueck mit --ps).
usage: spoolpdf.py SPOOL.GEO [--ps]"""
import sys
p = sys.argv[1]
d = bytearray(open(p, 'rb').read())
ps, pdf = b'\x06\x00.PS\x00', b'\x07\x00.PDF\x00'
old, new = (pdf, ps + b'\xcc') if '--ps' in sys.argv else (ps + b'\xcc', pdf)
if d.count(old) != 1:
    sys.exit('Muster nicht (eindeutig) gefunden - Datei unveraendert.')
i = d.find(old); d[i:i + len(old)] = new[:len(old)] if len(new) >= len(old) else new + d[i + len(new):i + len(old)]
open(p, 'wb').write(d)
print('geaendert an Offset %#x' % i)
