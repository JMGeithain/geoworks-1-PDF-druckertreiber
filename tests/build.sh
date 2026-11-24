#!/bin/sh
# Baut die drei K12-Testprogramme mit pcgeos-1-sdk-v2 (ohne Zielprofil-Pruefung,
# da UI.GEO/MOTIF.GEO der deutschen Installation vom Profil abweichen).
# usage: build.sh /pfad/zu/pcgeos-1-sdk-v2 [ausgabe]
set -e
SDK=$1; OUT=${2:-$(pwd)/out}
HERE=$(cd "$(dirname "$0")" && pwd)
for t in k12test1 k12test2 k12test3; do
  cp "$HERE"/common/*.inc "$HERE/$t/"
  perl "$SDK/tools/ensemble12/build.pl" --project "$HERE/$t" \
       --output-dir "$OUT/$t" --nasm "$(command -v nasm)" > "$OUT.$t.json"
  echo "$t -> $OUT/$t/$t.geo"
done
