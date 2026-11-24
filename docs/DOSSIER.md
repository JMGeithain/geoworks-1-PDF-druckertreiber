# Dossier: Ein PDF-Druckertreiber für GeoWorks Ensemble 1.2x

Stand: 22. November 2026, Treiberversion 1.2a

## 1. Ziel und Ergebnis

**Ziel** war ein eigener PDF-Druckertreiber für GeoWorks Ensemble 1.x, wie ihn
Konstantin Meyer (HubertHuckevoll) gerade für FreeGEOS baut. GeoWorks 1.x bringt
einen PostScript-Treiber mit (`PS.GEO`). Ein PDF entstand bisher nur auf dem Umweg
über eine .PS-Datei und Ghostscript oder Ähnliches außerhalb von GEOS.

**Ergebnis:** `PDF.GEO` ist ein vollwertiger Druckertreiber für GeoWorks 1.2/1.28:
- Er erscheint in den Voreinstellungen als „PDF-Datei (In Datei drucken)“.
- Er schreibt über „In Datei Drucken…“ direkt eine gültige PDF-1.4-Datei.
- Er ist mit Dokumenten aus GeoWrite, GeoDraw und GeoPlanner am echten System
  getestet, darunter Briefe, Kalender, Plakate, Urkunden und eine 8-seitige
  technische Anleitung mit Bildschirmfotos.
- Die Seiten sehen aus wie am Bildschirm, einschließlich der Zierschriften, Bitmaps
  und Muster.

Der Treiber ist in 8086-Assembler geschrieben (NASM, rund 4 400 Zeilen). Dazu kommt ein
eigenes Bauprogramm für 1.x-Geodes, denn das verfügbare SDK erzeugt keine Treiber.
Getestet wurde zweistufig:
1. jede Version zuerst in einem 8086-Emulator mit nachgebildeten Kernel-Aufrufen,
2. danach am echten GeoWorks unter DOSBox.

## 2. Ausgangslage

| | |
|---|---|
| System | deutsches GeoWorks Ensemble 1.2 / 1.28 unter DOSBox |
| Vorhandene Druckertreiber | `PS.GEO` (PostScript), `LASERJET.GEO`, Nadeldrucker |
| Dokumentation zu 1.x | keine. Die FreeGEOS-Quellen beschreiben 2.x/3.x und weichen in vielen Strukturen ab |
| Werkzeuge | pcgeos-1-sdk-v2 (dnesbitt142) für 1.x-Anwendungen, aber nicht für Treiber; FreeGEOS-Quellen als Vergleich |

Die Grundidee stammt vom FreeGEOS-Treiber von Konstantin Meyer: Der Treiber nimmt den
Grafik-String (GString) einer Seite entgegen und übersetzt ihn Element für Element in
PDF-Befehle. Der Spooler von 1.x bietet diesen Weg schon für PostScript an (die
„PDL“-Schiene). Der PDF-Treiber hängt sich an dieselbe Stelle.

## 3. Vorgehen und Chronik

| Schritt | Inhalt |
|---|---|
| Erforschung | Die Spooler-, PostScript- und LaserJet-Geodes wurden mit eigenem Geode-Leser und Disassembler untersucht: Treiberschnittstelle, PrinterInfo, PDL-Weg. |
| Befehlstabelle | Auf Wunsch zuerst vollständig ausgezogen: alle 90 GString-Opcodes von 1.2 aus `PS.GEO` und `KLIB.GEO`, abgeglichen mit den 1.x-Tabellen der FreeGEOS-Konvertierungsbibliothek (`docs/GSTRING12.md`). |
| Kernel-Namen | Die Einsprungtabelle von `KERNEL.EXE` (577 Einträge) wurde über die KLIB-Abspieltabelle, DOS-Aufrufe und Vergleiche benannt (`docs/KERNEL12.md`, `data/kernel12.json`). Drei Testprogramme (K12 Test 1–3) haben die unsicheren Aufrufe am echten System bestätigt. |
| M0 | Minimaler Treiber: lädt, erscheint in den Voreinstellungen, schreibt ein leeres, gültiges PDF. |
| M1 | Linien, Rechtecke, Polygone, Ellipsen. Jedes Element bekommt seine eigene Transformation (GeoDraw zeichnet Einheitsformen mit Objektmatrix). Textfelder kommen dazu. |
| M2 | Schriftzuordnung: Familie aus der FontID; fett und kursiv. |
| M3 | Unter- und Durchstreichen. Zeilenbreite wie in GEOS über Zeichenabstand (`Tc`). |
| M4 | Hoch- und Tiefstellung. Grundlinie über das Bit `TM_DRAW_BASE`. |
| M5 | Draw-Masken (Graumuster) als Aufhellung der Farbe. |
| M6 | Bitmaps, Symbol- und Dingbats-Schriften, Tabulatoren. Spooler-Patch .PS → .PDF. |
| M7 | Maske 0 % = durchsichtig: Element wird nicht gezeichnet. |
| M8 | Eingebettete Schriften: Nimbus-Umrisse aus `FONT\*.FNT` als Type3-Schriften. |
| 1.0 | Aufräumen: keine Diagnose-Kommentare mehr, Glyphen werden stückweise gelesen (auch Zeichen über 4 KB), kleinere DGROUP, Anleitung. |
| 1.1 | Flächenmuster als PDF-Kachelmuster (Bits aus `KERNEL.EXE`). Querformat mit `/Rotate`. |
| 1.2 | In GeoWrite eingefügte Grafiken: in Textfelder eingebettete GStrings. |
| 1.2a | Absturz „bad handle passed to system“ behoben (Registerbelegung von `GrDestroyGString` in 1.x). |

## 4. Erkenntnisse über das Drucksystem von GeoWorks 1.x

Die vollständigen Tabellen stehen in `DRIVER12.md`, `GSTRING12.md` und
`KERNEL12.md`. Hier die Kernpunkte.

### 4.1 Spooler und Treiber

- Die Strategie-Routine erhält in **DI** die Funktion. Ab DI = 6 steht in BX das
  PState-Handle. Seitenweise ruft der Spooler auf: 2Eh Seite beginnen,
  **8100h Seitentransformation**, GString laden, **8101h GString drucken**,
  GString freigeben, 30h Seite beenden.
- Der Spooler nimmt den Grafik-String-Weg nur, wenn in der Geräte-Info
  `smarts & 3 = 2` gilt (wie bei PostScript).
- **Drucken in Datei:** Das geht über den Knopf „In Datei Drucken…“ im Druckdialog der
  Anwendung, nicht über die Schnittstelle in den Voreinstellungen.
  - Die `port`-Auswertung in `GEOS.INI` kennt nur `L…` (parallel), alles andere gilt
    als seriell.
  - Der Dateidialog hängt fest „.PS“ an. Das ist ein Text-Chunk in Ressource 25 von
    `SPOOL.GEO`; im Speicherblock war ein Füllbyte frei, sodass „.PDF“ ohne
    Größenänderung passt.
- **8101h** muss SI unverändert zurückgeben. AX = 1 bedeutet Seitenwechsel, AX = 0 Ende.

### 4.2 Abspielen der GStrings

- `GrDrawGString` mit **BP = 4800h** hält vor jedem Ausgabe-Element an
  (BP = 4, DL = Opcode). Attribut-Elemente führt GEOS dabei still im Ziel-GState aus.
- Am Ende kommt BP = FFh zurück.
- Der Treiber kopiert jedes Ausgabe-Element mit `GrCopyGString` (CX = 2000h) in einen
  eigenen Chunk und erfragt die Attribute vom Hilfs-GState: Farben, Masken,
  Linienbreite, Schrift, Stil, Stiftposition und Transformation.
- **`GrDestroyGString` erwartet in 1.x das GString-Handle in DI.** Der Spooler macht es
  selbst so: `mov di,si; mov dl,1`. In 2.x steht es in SI. Diese Verwechslung
  verursachte den Absturz vor Version 1.2a.

### 4.3 Text

- **Textfelder** (Opcode 21h) bestehen aus drei Teilen:
  - `fcount`, dann der gesicherte Feldkopf (28 Byte): Position, Breite, textLeft,
    adjustment, yPos, baseLine usw.,
  - die Zeichen,
  - die Abschnitte: Zähler plus 20-Byte-`OldTextAttr` mit Farbe, Maske, Stil, Modus,
    FontID und Größe.
- **`TM_DRAW_BASE`** (Modus-Bit 08h) bedeutet: yPos ist die Grundlinie. Ohne dieses Bit
  ist yPos die Oberkante.
- **FontID:** Hersteller = ID & F000h (0 = Bitmap-Schrift), Familie = (ID >> 9) & 7
  (0 Serif, 1 Sans, 4 Symbol, 5 Mono …).
- Hoch- und Tiefstellung erscheinen in halber Größe, verschoben um −0,375 bzw. +0,1
  der Schriftgröße (wie im EPS-Export von FreeGEOS).

### 4.4 Masken und Muster

- `SystemDrawMask`: 0–24 sind Muster, 25 bis 89 Graustufen von 100 bis 0 %.
- Die Bitmuster stehen als 90 × 8 Byte in `KERNEL.EXE` (Image-Offset 0EAB2h).
- Getter: `GrGetLineMask`, `GrGetAreaMask` und `GrGetTextMask` (262/263/264), jeweils
  mit AL = 0.

### 4.5 Bitmaps

- Ein `Bitmap`-Kopf hat 6 Byte. Komplexe Bitmaps (Typ-Bit 08h) haben einen 20-Byte-Kopf
  mit startScan, numScans, Daten- und Palettenoffset sowie Auflösung.
- Große Bilder kommen in mehreren **Scheiben** hintereinander. Die letzte ist erreicht,
  wenn startScan + numScans die Höhe ergibt.
- Kompression: PackBits je Zeile.

### 4.6 Eingebettete Grafiken in GeoWrite

- Eine eingefügte Grafik ist im Text das Zeichen 1Ah.
- Im Druck-GString hängt hinter dem letzten Textabschnitt des Feldes ein
  **vollständiger 1.x-GString**. Er beginnt mit `GR_SAVE_STATE`, enthält Escapes
  4100h–410Ah, Füllrechteck und Bitmap und endet mit `GR_END_STRING`.
- Der Ursprung liegt bei x = Feld-x, y = yPos − baseLine (bei `TM_DRAW_BASE`).
- In den Quellen von FreeGEOS ist dieser Fall nur als „nicht unterstützt“ vermerkt.
  Er wurde hier aus einem Diagnose-Ausdruck entschlüsselt.

### 4.7 Schriftdateien (`FONT\*.FNT`)

- **Dateikopf:** „BSWF“, Version, Kopfgröße, FontInfo ab Offset 6 (fileHandle, FontID,
  Hersteller, Familie, Name[20], Tabellen-Offsets).
- **Umrisstabelle:** Einträge zu 26 Byte (Stil, Gewicht, 3 × OutlineEntry):
  - Kopf mit Maßen und Breiten (3 Byte je Zeichen),
  - Teil 1 mit den Zeichen unter 80h,
  - Teil 2 mit den Zeichen ab 80h.
- Jeder Teil beginnt mit einer Offsettabelle.
- **Zeichen:** Begrenzung (4 Wörter), x- und y-Tripel (werden übersprungen), dann die
  Nimbus-Befehle:
  - 0 move, 1 line, 2 bezier, 3 Ende,
  - 5 Akzent-Kompositum,
  - 6/7 vertikale/horizontale Linie,
  - 8 relative Linie (Bytes y, x),
  - 9 relative Kurve.
- Bezug: `Driver/Font/Nimbus` in FreeGEOS. Für Prüfungen am PC gibt es den Leser
  `fonts/fnt12.py`.
- Alle 99 Umrissschriften der Testinstallation werden fehlerfrei gelesen. Das größte
  Zeichen (ReliefDeco) hat 5,6 KB Umrissdaten.

### 4.8 Weitere Einzelheiten

- `FileSetStandardPath` mit AX = 0 setzt das GEOS-Hauptverzeichnis.
- `FileOpen` mit AL = C0h öffnet zum Lesen.
- Der Treiber durchsucht `FONT\*.FNT` mit DOS-FindFirst/FindNext (INT 21h, eigene DTA).
  Das funktioniert unter GeoWorks 1.x zuverlässig.
- Im SDK-Profil ist Eintrag 324 als `GrDrawEllipse` geführt. Tatsächlich ist 324
  `GrDrawPoint`, die Ellipse ist 333.

## 5. Aufbau des Treibers

### 5.1 Dateien

| Datei | Zeilen | Inhalt |
|---|---:|---|
| `driver/pdfdrv.asm` | 1 250 | Strategie, Auftrag/Seite, PDF-Objekte, Ausgabepuffer, GString-Schleife, Attributabfrage |
| `driver/emit.inc` | 1 700 | Übersetzung der Elemente: Formen, Text, Textfelder, Bitmaps, Masken |
| `driver/fonts.inc` | 1 100 | Schriftsuche, Schriftplätze, Type3-Ausgabe, Nimbus-Interpreter |
| `driver/embed.inc` | 210 | eingebettete GStrings in Textfeldern, Matrixverkettung |
| `driver/pattern.inc` | 160 | Kachelmuster |
| `driver/*.inc` (erzeugt) | | Breitentabellen (Adobe-Metriken), Glyphennamen, Musterbits |
| `driver/build_driver.py` | | baut die Geode: 5 Ressourcen, Relocations, Treibertabelle, Geräte- und Treiber-Info |
| `driver/emutest.py` | | 8086-Emulator-Test mit nachgebildeten Kernel- und DOS-Aufrufen |

### 5.2 Ressourcen der Geode

| Nr. | Inhalt |
|---|---|
| 0 | Core |
| 1 | DGROUP (14,5 KB): Variablen, Ausgabepuffer, Schriftverzeichnis, 8 Schriftplätze, Lesepuffer, Objekttabelle für 1 200 PDF-Objekte |
| 2 | Code |
| 3 | Geräte-Info |
| 4 | Treiber-Info |

### 5.3 PDF-Aufbau

| Objekt | Inhalt |
|---|---|
| 1 | Katalog |
| 2 | Seitenbaum |
| 3 | gemeinsames Ressourcen-Objekt: Standardschriften F1–F14, eingebettete Schriften F15–F22, Muster P1–P24, Farbraum `CsP` |
| 4 + 3i … 6 + 3i | je Seite i: Inhalt, Länge, Seite |
| danach | Type3-Schriften (je Zeichen Strom + Länge) und Muster |

Die Seiteninhalte stehen in GEOS-Koordinaten. Die Umrechnung erledigt
`1 0 0 -1 0 H cm` am Seitenanfang. Jedes Element wird mit seiner eigenen Matrix
umgerechnet.

### 5.4 Schriften

1. URW Roman, Sans und Mono (1000h/1200h/1A00h) sowie SymbolPS (1800h) werden auf
   Times, Helvetica, Courier und Symbol abgebildet, mit den Breiten aus den
   Adobe-Metriken.
2. Alle anderen Umrissschriften werden beim ersten Gebrauch im FONT-Verzeichnis gesucht.
   Der Treiber liest Kopf und Breiten und merkt sich die benutzten Zeichen.
   Am Auftragsende schreibt er je Schrift eine Type3-Schrift:
   - Glyphennamen nach der Adobe-Liste (Mac-Roman), damit der Text durchsuchbar bleibt,
   - fehlendes Kursiv durch 12° Schräge der Textmatrix,
   - fehlendes Fett durch Füllen plus Umriss.
3. Bildschirm-Bitmapschriften und Druckerschriften werden durch Standardschriften
   ersetzt.

Die Zeilenbreite gleicht der Treiber über `Tc` an die Breite an, die GEOS im
Textfeld angibt. Dadurch stehen Tabulatoren, Spalten und Zeilenenden wie am Bildschirm.

## 6. Tests

### 6.1 Emulator

`emutest.py` lädt `PDF.GEO` wie GEOS: Ressourcen, Relocations und Kernel-Einsprünge
als Fallen. Dann ruft es die Treiberfunktionen in der Reihenfolge des Spoolers auf.

Nachgebildet sind:
- Speicher- und LMem-Verwaltung,
- GString-Kopie und -Abspielen, auch verschachtelt,
- Attribut-Getter,
- Datei- und DOS-Aufrufe für die Schriftsuche.

Ungültige GState- und GString-Handles lösen einen Fehler aus. So ließ sich der Absturz
aus 1.2 nachstellen. Das Ergebnis-PDF wird mit qpdf, pypdf und PyMuPDF geprüft und
gerendert.

### 6.2 Am echten System

Alle Ausdrucke stammen vom deutschen GeoWorks 1.28 unter DOSBox. Danach wurde jeweils
der Treiber verbessert.

| Dokument | Programm | geprüft |
|---|---|---|
| TestDraw | GeoDraw | Formen, Farben, Text in drei Schriften, Stile, Umlaute |
| Druckertest | GeoWrite | Mono/Roman, alle Stile, Tabulatorspalten, 7–100 pt |
| Kalender Monat/Jahr | GeoPlanner | Raster, Graumuster, KW-Spalte des GeoPlanner-Patches |
| Auto-Guthannss | GeoDraw | Querformat, Vagabond, Ping-Pong-Kästchen, Bitmap (Auto) |
| Urkunde, Gemeindebriefe, Gebühren | GeoDraw/GeoWrite | Rahmen, Broschürenseiten, Symbole |
| Plakat | GeoDraw | 16 Kachelseiten, große Schriften |
| Glückwunschkarte (BUNT) | GeoDraw | durchsichtige Flächen, gedrehter Text |
| Bereichstest | GeoDraw | Gittermuster |
| Fraktur, Bibelkuchen | GeoWrite | Fette Fraktur, Brüche hoch/tief |
| CCom Instructions (8 S.) | GeoWrite | eingefügte Bildschirmfotos in Textzeilen |

## 7. Grenzen und offene Punkte

- **Nicht umgesetzte Elemente:** Runde Rechtecke, Bögen, Splines, Punkte sowie die
  „_TO“-Varianten ab Stiftposition (0Eh, 10h, 12h, 14h, 29h). Bisher ist keins davon in
  einem Ausdruck aufgetaucht. Kommt eins vor, schreibt der Treiber eine kurze
  Kommentarzeile ins PDF.
- **Muster:** Muster bei Linien und Text erscheinen als Grauton.
- **Bildschirmschriften** (Berkeley, University, Bison, LED) werden ersetzt statt
  eingebettet.
- **Fette Fraktur:** Die Schriftdatei enthält keine Umlaute und kein ß, und auch der
  Bildschirm zeigt sie nicht an. Ein Nachbau der Schrift ist vorgemerkt.
- **Textzeilen mit Grafik und Text gemischt:** Noch kein Beispiel geprüft. Die Grafik
  wird am Zeilenanfang angenommen.
- **Zeichenabstand:** In sehr kurzen Zeilen mit großer GEOS-Breite fällt der
  Ausgleich über den Zeichenabstand auf (z. B. „P a g e“ in einer Kopfzeile).
  Abhilfe wäre, nur die Wortabstände zu strecken.
- **Grenzwerte:** höchstens 100 Seiten, 8 eingebettete Schriftschnitte und 1 200
  PDF-Objekte je Auftrag; 128 Schriftdateien im FONT-Verzeichnis.
- **Speicher:** Die DGROUP steht vollständig in der Datei (rund 14 KB Nullen), weil
  1.x keine erst beim Laden angelegten Ressourcen kennt.

## 8. Bauen, Prüfen, Installieren

```sh
./build.sh                                   # dist/PDF.GEO aus driver/pdfdrv.asm
cd driver
GEOWORKS_DIR=/pfad/zu/GEOWORKS python3 emutest.py ../dist/PDF.GEO /tmp/test.pdf
python3 gen_widths.py                        # Breitentabellen neu erzeugen (reportlab, fontTools)
```

Die Installation beschreiben `README.md` und `dist/LIESMICH.TXT`.

Spooler-Patch: `python3 spool/spoolpdf.py GEOWORKS/SYSTEM/SPOOL.GEO`. Vorher eine
Sicherung anlegen.

Hängt ein Druckauftrag: GeoWorks beenden und `GEOWORKS\SYSTEM\SPOOL\*` löschen.

## 9. Quellen

- FreeGEOS (bluewaysw/pcgeos):
  - `Library/Convert` (GString-Tabellen 1.x),
  - `Driver/Font/Nimbus` (Umrissformat),
  - `Include/graphics.def`, `fontID.def`, `printDr.def`,
  - EPS-Export (Hoch- und Tiefstellung).
- Konstantin Meyer (HubertHuckevoll): nativer PDF-Treiber für FreeGEOS, Zweig
  `native-pdf-printer-1160`.
- dnesbitt142/pcgeos-1-sdk-v2: SDK und Kernel-Profil für GeoWorks 1.x.
- Adobe Font Metrics der 14 Standardschriften (über reportlab).
- Die Glyphennamen folgen der Adobe Glyph List.
- Original-Geodes der Installation (nur untersucht, nicht weitergegeben): `SPOOL.GEO`,
  `PS.GEO`, `LASERJET.GEO`, `KLIB.GEO`, `KERNEL.EXE`.
