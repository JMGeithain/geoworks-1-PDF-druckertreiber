# PDF-Druckertreiber für GeoWorks Ensemble 1.2x

Ein eigener Druckertreiber für **GeoWorks Ensemble 1.2 / 1.28** (PC/GEOS 1.x), der jeden
Ausdruck als **PDF-Datei** schreibt: über „In Datei drucken…“, ohne Umweg über
PostScript und ohne zusätzliche Software unter DOS.

Vorbild ist der native PDF-Treiber, den Konstantin Meyer (HubertHuckevoll) für
[FreeGEOS](https://github.com/bluewaysw/pcgeos) entwickelt. Für GeoWorks 1.x gab es
dafür weder Quelltext noch Dokumentation. Das Drucksystem wurde deshalb aus den
Original-Geodes erschlossen. Der Treiber ist in 8086-Assembler geschrieben und mit
einem eigenen Geode-Bauprogramm erzeugt.

| | |
|---|---|
| Treiber | `dist/PDF.GEO` (Version 1.2a, ≈ 30 KB) |
| Gerät in den Voreinstellungen | „PDF-Datei (In Datei drucken)“ |
| Getestet mit | deutschem GeoWorks Ensemble 1.28 unter DOSBox: GeoWrite, GeoDraw, GeoPlanner |
| Ausführliche Beschreibung | [`docs/DOSSIER.md`](docs/DOSSIER.md) |

## Was der Treiber kann

- Linien, Rechtecke, Polygone und Ellipsen, auch gedreht und skaliert
- Farben, Graumuster als Grautöne, durchsichtige Flächen
- Flächenmuster (Gitter, Linien, Ziegel …) als echte PDF-Kachelmuster
- Bitmaps (s/w, 16 und 256 Farben, RGB, auch gepackt und in mehreren Scheiben)
- in GeoWrite eingefügte Grafiken (Grafik als Zeichen im Text)
- Text:
  - URW Roman, Sans, Mono und SymbolPS werden auf die PDF-Standardschriften abgebildet.
  - Alle anderen Umrissschriften aus `GEOWORKS\FONT` werden als Umrisse eingebettet,
    und zwar nur die benutzten Zeichen.
  - Fett und kursiv werden nachgebildet, wenn die Schrift sie nicht enthält.
  - Unterstrichen, durchgestrichen, hoch- und tiefgestellt.
  - Zeilenbreiten wie am Bildschirm, Tabulatoren.
  - Der Text bleibt im PDF durchsuchbar.
- mehrere Seiten; Querformat wird im PDF-Betrachter aufrecht angezeigt

## Installation (Kurzfassung)

1. `dist/PDF.GEO` nach `GEOWORKS\SYSTEM` kopieren (dort liegt auch `PS.GEO`).
2. In den Voreinstellungen unter *Drucker* „PDF-Datei (In Datei drucken)“ installieren.
   Die Schnittstelle ist egal, „Testen“ meldet „off-line“.
3. Im Druckdialog eines Programms **„In Datei Drucken…“** wählen.

Der Dateidialog des Spoolers hängt fest `.PS` an. Mit
[`spool/spoolpdf.py`](spool/spoolpdf.py) lässt sich die eigene `SPOOL.GEO` auf `.PDF`
umstellen (`--ps` macht das rückgängig). Die gepatchte Originaldatei liegt aus
rechtlichen Gründen nicht im Repository.

Ausführlich: [`dist/LIESMICH.TXT`](dist/LIESMICH.TXT) (DOS-tauglich, ohne Umlaute).

## Bauen

```sh
pip install capstone unicorn pypdf     # nur für Werkzeuge und Tests nötig
./build.sh                             # -> dist/PDF.GEO   (benötigt python3 + nasm)
cd driver && python3 emutest.py ../dist/PDF.GEO /tmp/test.pdf
```

`emutest.py` führt den fertigen Treiber in einem 8086-Emulator (Unicorn) aus. Dabei
werden die benutzten Kernel-Aufrufe nachgebildet. Mit `GEOWORKS_DIR=…` findet der Test
auch die Schriftdateien einer Installation.

## Aufbau des Repositorys

```
driver/   Treiber-Quelltext (NASM), Geode-Bauprogramm, Emulator-Test
docs/     DOSSIER.md (Gesamtbericht), DRIVER12.md (Treiber-Schnittstelle),
          GSTRING12.md (GString-Befehlscodes 1.2), KERNEL12.md (Kernel-Einsprünge)
include/  gstr12.inc/.h, kern12.inc/.h – Konstanten für eigene 1.x-Programme
data/     kernel12.json – Kernel-Einsprungnummern mit Namen und Sicherheit
tools/    Geode-Leser und -Disassembler, Auszug der Kernel- und Opcode-Tabellen
fonts/    fnt12.py – Leser für GeoWorks-1.x-Schriftdateien (Nimbus-Umrisse)
tests/    K12-Testprogramme für das pcgeos-1-sdk-v2 (Kernel-Aufrufe am echten System)
spool/    spoolpdf.py – Endung .PS -> .PDF im Spooler-Dialog
dist/     PDF.GEO und LIESMICH.TXT
```

## Danksagung und Quellen

- Konstantin Meyer (HubertHuckevoll): nativer PDF-Treiber für FreeGEOS, Vorbild und Anstoß
- [FreeGEOS / pcgeos](https://github.com/bluewaysw/pcgeos): GString-Tabellen der
  Konvertierungsbibliothek, Nimbus-Schrifttreiber, Include-Dateien
- [pcgeos-1-sdk-v2](https://github.com/dnesbitt142/pcgeos-1-sdk-v2) (dnesbitt142):
  SDK für GeoWorks 1.x, genutzt für die Testprogramme
- Adobe Font Metrics der 14 PDF-Standardschriften (Breitentabellen)

GeoWorks, GeoWrite, GeoDraw und GeoPlanner sind Marken ihrer jeweiligen Inhaber. Das
Repository enthält keine Original-Dateien von GeoWorks.

## Lizenz

*Noch festzulegen.*
