# Druckertreiber-Schnittstelle von GeoWorks 1.2

Stand 20.11.2026. Abgeleitet aus `SPOOL.GEO`, `PSCRIPT.GEO`, `LASERJET.GEO`;
umgesetzt in `driver/pdfdrv.asm` und `driver/build_driver.py`.

## Geode

* Kopf wie jede 1.x-Geode (120h Byte). Attribute `3000h`, Typ 3, Token `PRDR`,
  Protokoll 1.0, Kernel 622.3. **+108h/+10Ah = Offset/Ressource der Treibertabelle**
  (bei allen Druckertreibern 0/1).
* Ressourcen: 0 Core (leer, 0880h), 1 DGROUP (fest, 0080h), 2 Code (fest, 0AC0h),
  Geräte-Info(s) (0972h), Treiber-Info als LMem-Block (097Ah).
* **Treibertabelle** (DGROUP +0): Strategie Offset, Strategie Segment (Relocation
  SEG), Attribute `2000h`, Typ `9` (Drucker), Handle des Treiber-Info-Blocks
  (Relocation HANDLE).
* Relocations: 00h Kernel-Far-Pointer (Operand = Einsprungnummer),
  22h Segment einer eigenen Ressource, 23h Handle einer eigenen Ressource
  (Operand = Ressourcennummer), 14h Bibliotheksaufruf.

## Strategie

DI = Funktion. Ab DI = 6 übergibt der Aufrufer in BX das PState-Handle; der
Treiber sperrt es und arbeitet mit dem Segment (PSCRIPT: BP).

| DI | Funktion | Ein / Aus |
|---:|---|---|
| 00 / 02 | Init / Exit | – |
| 04 | Gerät testen | → AX |
| 06 | Gerät setzen | DX:SI Gerätename; PState+1 = Index, PState+3 = Handle der Geräte-Info |
| 08 | Treiber-Info | → DX:SI = Handle:0 des LMem-Blocks |
| 0A | Geräte-Info | → DX:SI = PState+3 : 0 |
| 0C | Modus setzen | CL Modus, DL/DH Papier, AX Breite, SI Höhe (Punkte) → PState+0/7/8/9/0Bh |
| 0E | Ausgabeziel | CX Token bzw. **Datei-Handle**, DX Stream-Treiber, SI Porttyp (**4 = Datei**) → PState+17h/19h |
| 10 | Druckbereich | → AX links, SI oben, CX rechts, DX unten |
| 12 / 14 / 16 / 18 | Modus-/Stilabfrage, Cursor holen, Cursor auf 0 | |
| 1A | Auftrag beginnen | DX:SI Auftragsparameter |
| 1C | Auftrag beenden | |
| 1E–2A | Text-/Stilfunktionen (Textmodus) | |
| 2C | Cursor setzen | CX, DX |
| 2E | Seite beginnen | CL |
| 30 | Seite beenden | CL |
| 8100h | Seitentransformation | DX:SI = TransMatrix (6 × WWFixed) |
| 8101h | GString drucken | SI = GString, CX = Flags → AX: 1 = Seitenwechsel, 0 = Ende; SI muss erhalten bleiben |

Der Spooler nimmt den PDL-Weg, wenn Byte +2 der Geräte-Info (Smarts) & 3 = 2 ist.
Ablauf je Seite: 2E, 8100h, GString laden (`GrLoadGString`), 8101h, GString freigeben, 30.

## Geräte-Info (PrinterInfo, 1.x)

| Offset | Inhalt |
|---:|---|
| 0 | Typ (Farbformat) |
| 1 | Anschlüsse |
| 2 | Smarts (0 Raster, 2 PDL) |
| 3, 5, 7 | Zeiger auf Grafik-Eigenschaften für niedrig/mittel/hoch (0 = nicht vorhanden) |
| 9, 0Bh | Zeiger für Text Entwurf/NLQ |
| 0Dh–13h | Ränder links, oben, rechts, unten |
| 15h, 16h | Papierzufuhr, Papierklasse |
| GP | xres, yres, Bandhöhe, Byte, Byte Farbformat |

## Treiber-Info (LMem-Block)

16-Byte-LMem-Kopf (Handle, Offset Handle-Tabelle, Flags, Typ, Blockgröße,
Anzahl Handles, Freiliste, frei), dann bei +10h: Anzahl Geräte, Offset
Namensliste, Offset Infoliste, Timeout, Flags, Chunk der ASCII-Tabelle
(`;;`). Namensliste = Chunk-Handles, mit 0 abgeschlossen; Infoliste = Handles
der Geräte-Info-Ressourcen. Chunks liegen auf 4n+2, ihr Längenwort zählt
sich selbst mit.

## Eingebettete Schriften (ab Meilenstein 8)

- URW Roman/Sans/Mono/SymbolPS (1000h/1200h/1A00h/1800h), Bitmap- und Druckerschriften
  werden weiter auf die PDF-Standardschriften abgebildet.
- Alle anderen Umrissschriften werden als Type3-Schrift eingebettet:
  - Beim ersten Gebrauch sucht der Treiber per DOS (AH=4Eh/4Fh) in `FONT\*.FNT`
    (relativ zu SP_TOP = 0) und merkt sich FontID -> Dateiname (bis 128 Dateien).
  - Aus der Datei werden die Umrisstabelle (26-Byte-Eintraege), der Kopf mit den
    Breiten sowie die Lage von Teil 1 (Zeichen < 80h) und Teil 2 (ab 80h) gelesen.
  - Am Auftragsende entsteht je Schrift ein Type3-Objekt mit den benutzten Zeichen.
    Je Zeichen gibt es einen Glyphenstrom aus den Nimbus-Befehlen 0/1/2/5/6/7/8/9,
    Akzent-Komposita werden aufgeloest.
  - Die Glyphennamen folgen der Adobe-Liste (Mac-Roman), damit Text durchsuchbar bleibt.
- Fehlt der gewuenschte Stil in der Datei, wird er nachgebildet:
  - kursiv ueber 12 Grad Schraegung der Textmatrix,
  - fett ueber Fuellen plus 40/1000 em Umriss (Operator B).
- Siehe auch `fonts/fnt12.py` (Python-Leser fuer dasselbe Format).

## Muster und Querformat (Version 1.1)

- GEOS-Draw-Masken 1..24 (Bits aus KERNEL.EXE, Tabelle bei Image-Offset 0EAB2h,
  8 Byte je Maske, 25..89 = Graustufen 100..0 %) werden bei Flaechen als
  ungefaerbte Kachelmuster `/CsP cs r g b /Pn scn` ausgegeben (8 x 8 pt).
- Ressourcen liegen jetzt komplett in Objekt 3 (`/Resources 3 0 R`).
- Seitentransformation (Escape 8100h) mit e12 > 0 -> `/Rotate 270`, e12 < 0 -> 90,
  e11 < 0 -> 180.

## Eingebettete Grafiken in Textfeldern (Version 1.2)

GeoWrite speichert eingefuegte Grafiken als Zeichen 1Ah. Im Druck-GString enthaelt das
Textfeld (Opcode 21h) dann hinter dem letzten Textabschnitt einen vollstaendigen
1.x-GString (beginnt mit GR_SAVE_STATE 04h, endet mit GR_END_STRING). Beobachtet
(CCom Instructions, 21.11.2026): Feld mit einem Zeichen 1Ah und Abschnittszaehler 0,
danach z. B. SAVE, TRANSLATE, INIT_DEFAULT_TRANSFORM, ..., Escapes 4100h-410Ah,
FILL_RECT (weisser Grund), DRAW_BITMAP (mehrere Scheiben), RESTORE, END.
Lage: Ursprung = (position + textLeft + adjustment, yPos - baseLine) bei TM_DRAW_BASE.
Der Treiber kopiert den GString in einen eigenen LMem-Block, laedt ihn mit
GrLoadGString (CL=0) und spielt ihn mit eigenem Hilfs-GState ab; die Feldmatrix wird
jeder inneren Matrix nachgeschaltet. Bitmaps enden nach der Scheibe, bei der
startScan + numScans die Hoehe erreicht.
