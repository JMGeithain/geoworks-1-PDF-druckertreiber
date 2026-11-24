#!/bin/sh
# Baut den PDF-Druckertreiber PDF.GEO fuer GeoWorks 1.2x.
# Voraussetzung: Python 3, NASM.  usage: ./build.sh [ausgabe.geo]
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${1:-$HERE/dist/PDF.GEO}
cd "$HERE/driver"
python3 build_driver.py pdfdrv.asm "$OUT" "$(command -v nasm)"
