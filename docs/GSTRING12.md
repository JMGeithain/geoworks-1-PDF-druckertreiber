# GString-Befehlscodes von GeoWorks 1.2

Referenz für den PDF-Druckertreiber. Stand 20.11.2026.

## Quellen und Prüfung

* **Opcode-Liste und Elementaufbau:** FreeGEOS `Library/Convert/convertGStringTables.asm`
  und `convertGString.asm/.def` (GeoWorks, Jim DeFrisco 1992). Diese Bibliothek
  wandelt 1.2x-Dokumente nach 2.0 um und spielt dafür jeden alten GString-Befehl
  einzeln ab. Die Position in `PlayElemTable` ist der 1.x-Opcode. Die Kommentare
  der Form `=5`, `=11` usw. in dieser Datei sind *andere* Nummern und gelten hier nicht.
* **Gegenprobe am Binary:** Die Sprungtabelle in `SYSTEM\PS.GEO` (Ressource 5,
  Offset `0cd8h`, Index = Opcode − 0Dh) deckt genau die Ausgabe-Opcodes 13–46 ab.
  Alle Handler-Gruppen und die Unterscheidungen innerhalb der Handler passen
  (`tools/verify_optable.py`, Ergebnis „konsistent“).
* Die Kennungen `GR12X_*` in `convertGString.def` (HLINE=17, HLINE_TO=18,
  VLINE_TO=20, BITMAP_CP=26, CHAR_CP=30, TEXT_CP=32, POLYLINE=35, POLYGON=39)
  stimmen mit der Liste überein.

**Am System bestätigt (K12 Test 2/3, 20.11.2026):** Ein aufgezeichneter GString
enthielt genau `4E 01 | 0D x1 y1 x2 y2 | 50 04 | 28 l t r b | 46 00 0E 00 02 02 |
1F x y 02 00 'A' 'b' | 1D 'Z' x y | 24 l t r b | 00`. Damit sind die Layouts von
SET_LINE_COLOR_INDEX, DRAW_LINE, SET_AREA_COLOR_INDEX, FILL_RECT, **SET_FONT**
(WBFixed-Größe, dann FontID), DRAW_TEXT, **DRAW_CHAR** (Zeichen zuerst) und
DRAW_ELLIPSE bestätigt. Farben mit Index werden als 4E/50 gespeichert, nicht als
RGB. Das LMem-Längenwort eines Chunks zählt seine eigenen 2 Byte mit.
Weiter ungeprüft: die übrigen Attribut-Opcodes und `OldTextAttr` (19/20 Byte).

**Abspielprotokoll (bestätigt):** `GrDrawGString` (283) mit BP=4800h hält vor
jedem Zeichenbefehl an und liefert BP=0004h und DL=Opcode. Attribut-Opcodes führt
der Kernel dabei auf dem Ziel-GState aus, sie werden nicht gemeldet. Am Ende
kommt BP=00FFh, DL=00. Danach entweder BP=2000h (ein Element ausführen) oder
`GrCopyGString` (204, CX=2000h) aufrufen; beides rückt weiter.

## Elementaufbau allgemein

* 1 Byte Opcode, danach die Argumente, ohne Längenfeld. Die Länge ergibt sich
  aus dem Opcode (bei Text, Polygonen und Bitmaps aus einem Zählerfeld).
* Koordinaten sind `sword` (16 Bit, Dokumentkoordinaten), Little Endian.
* `WWFixed` = 2 Byte Bruchteil + 2 Byte ganzzahlig. `WBFixed` = 1 Byte Bruchteil + 2 Byte ganzzahlig.
* „Ausgabe“-Opcodes sind 13–46. Nur bei diesen hält `PS.GEO` das Abspielen an
  (Kernel-Eintrag 283, vermutlich `GrDrawGString`, mit Steuer-Flags in BP;
  der Opcode kommt in DL zurück). Alle anderen Opcodes führt der Kernel selbst
  im GState aus. Der Übersetzer liest danach die aktuellen Attribute aus dem GState.

## Tabelle

Spalte **PS.GEO**: wie die PostScript-Bibliothek den Opcode behandelt.
„übersprungen“ heißt: Das Element wird geholt und ohne Ausgabe verworfen.
„Kernel“ heißt: Der Opcode wird nie an den Übersetzer gemeldet.

| Dez | Hex | Name | Argumente nach dem Opcode | PS.GEO |
|---:|---:|---|---|---|
| 0 | 00 | GR_END_STRING | – | Ende |
| 1 | 01 | GR_COMMENT | `word n`, n Bytes | Kernel |
| 2 | 02 | GR_NULL_OP | – | Kernel |
| 3 | 03 | GR_ESCAPE | `word code`, `word n`, n Bytes | Kernel |
| 4 | 04 | GR_SAVE_STATE | – | Kernel |
| 5 | 05 | GR_RESTORE_STATE | – | Kernel |
| 6 | 06 | GR_NEW_PAGE | – | Kernel / Seitenende |
| 7 | 07 | GR_APPLY_ROTATION | `WWFixed winkel` (4) | Kernel |
| 8 | 08 | GR_APPLY_SCALE | `WWFixed x, y` (8) | Kernel |
| 9 | 09 | GR_APPLY_TRANSLATION | `WWFixed x, y` (8) | Kernel |
| 10 | 0A | GR_SET_TRANSFORM | `OldTransMatrix` (24) | Kernel |
| 11 | 0B | GR_APPLY_TRANSFORM | `OldTransMatrix` (24) | Kernel |
| 12 | 0C | GR_SET_NULL_TRANSFORM | – | Kernel |
| 13 | 0D | GR_DRAW_LINE | `x1, y1, x2, y2` (8) | Linien-Handler 05D9 |
| 14 | 0E | GR_DRAW_LINE_TO | `x, y` (4) | 05D9 |
| 15 | 0F | GR_DRAW_RECT | `l, t, r, b` (8) | Rechteck 08BF |
| 16 | 10 | GR_DRAW_RECT_TO | `x, y` (4) | 08BF |
| 17 | 11 | GR_DRAW_HLINE | `x1, y, x2` (6) | 05D9 |
| 18 | 12 | GR_DRAW_HLINE_TO | `x2` (2) | 05D9 |
| 19 | 13 | GR_DRAW_VLINE | `x, y1, y2` (6) | 05D9 |
| 20 | 14 | GR_DRAW_VLINE_TO | `y2` (2) | 05D9 |
| 21 | 15 | GR_DRAW_ROUND_RECT | unbekannt (Convert: Fehler) | übersprungen |
| 22 | 16 | GR_DRAW_ROUND_RECT_TO | unbekannt | übersprungen |
| 23 | 17 | GR_DRAW_POINT | `x, y` (4) | übersprungen |
| 24 | 18 | GR_DRAW_POINT_CP | – | übersprungen |
| 25 | 19 | GR_DRAW_BITMAP | `x, y`, `word slice`, Bitmap | Bitmap-Res. 8 |
| 26 | 1A | GR_DRAW_BITMAP_CP | `word slice`, Bitmap | Res. 8 |
| 27 | 1B | GR_DRAW_BITMAP_PTR | `x, y`, `word offset` (6) | übersprungen |
| 28 | 1C | GR_DRAW_BITMAP_OPTR | `x, y`, `word chunk`, `word handle` (8) | übersprungen |
| 29 | 1D | GR_DRAW_CHAR | `byte ch`, `x, y` (5) | **übersprungen** |
| 30 | 1E | GR_DRAW_CHAR_CP | `byte ch` (1) | **übersprungen** |
| 31 | 1F | GR_DRAW_TEXT | `x, y, word n`, n Zeichen | Text-Res. 6:0A1E |
| 32 | 20 | GR_DRAW_TEXT_CP | `word n`, n Zeichen | Res. 6:0A1E |
| 33 | 21 | GR_DRAW_TEXT_FIELD | siehe unten | Res. 6:0000 |
| 34 | 22 | GR_DRAW_TEXT_PTR | `x, y`, `word offset` (6) | übersprungen |
| 35 | 23 | GR_DRAW_POLYLINE | `word n`, n × (`x, y`) | Poly 06D7 |
| 36 | 24 | GR_DRAW_ELLIPSE | `l, t, r, b` (8) | Ellipse 0C28 |
| 37 | 25 | GR_DRAW_ARC | unbekannt (Convert: Fehler) | übersprungen |
| 38 | 26 | GR_DRAW_SPLINE | `word n`, n × (`x, y`) | übersprungen |
| 39 | 27 | GR_DRAW_POLYGON | `word n`, n × (`x, y`) | 06D7 |
| 40 | 28 | GR_FILL_RECT | `l, t, r, b` (8) | Füllrechteck 096E |
| 41 | 29 | GR_FILL_RECT_TO | `x, y` (4) | 096E |
| 42 | 2A | GR_FILL_ROUND_RECT | unbekannt | übersprungen |
| 43 | 2B | GR_FILL_ROUND_RECT_TO | unbekannt | übersprungen |
| 44 | 2C | GR_FILL_ARC | unbekannt | übersprungen |
| 45 | 2D | GR_FILL_POLYGON | `byte regel`, `word n`, n × (`x, y`) | Füllpolygon 0763 |
| 46 | 2E | GR_FILL_ELLIPSE | `l, t, r, b` (8) | Füllellipse 0B99 |
| 47 | 2F | GR_SET_DRAW_MODE | `byte mixmode` | Kernel |
| 48 | 30 | GR_REL_MOVE_TO | `dx, dy` (4) | Kernel |
| 49 | 31 | GR_MOVE_TO | `x, y` (4) | Kernel |
| 50 | 32 | GR_SET_LINE_COLOR | `R, G, B` (3) | Kernel |
| 51 | 33 | GR_SET_LINE_MASK | `byte` | Kernel |
| 52 | 34 | GR_SET_LINE_COLOR_MAP | `byte` | Kernel |
| 53 | 35 | GR_SET_LINE_WIDTH | `word` (ganzzahlig) | Kernel |
| 54 | 36 | GR_SET_LINE_JOIN | `byte` | Kernel |
| 55 | 37 | GR_SET_LINE_END | `byte` | Kernel |
| 56 | 38 | GR_SET_LINE_ATTR | `OldLineAttr` (10) | Kernel |
| 57 | 39 | GR_SET_MITER_LIMIT | `WWFixed` (4) | Kernel |
| 58 | 3A | GR_SET_LINE_STYLE | `byte stil`, `byte index` (2) | Kernel |
| 59 | 3B | GR_SET_AREA_COLOR | `R, G, B` (3) | Kernel |
| 60 | 3C | GR_SET_AREA_MASK | `byte` | Kernel |
| 61 | 3D | GR_SET_AREA_COLOR_MAP | `byte` | Kernel |
| 62 | 3E | GR_SET_AREA_ATTR | `OldAreaAttr` (6) | Kernel |
| 63 | 3F | GR_SET_TEXT_COLOR | `R, G, B` (3) | Kernel |
| 64 | 40 | GR_SET_TEXT_MASK | `byte` | Kernel |
| 65 | 41 | GR_SET_TEXT_COLOR_MAP | `byte` | Kernel |
| 66 | 42 | GR_SET_TEXT_STYLE | `byte setzen`, `byte löschen` | Kernel |
| 67 | 43 | GR_SET_TEXT_MODE | `byte setzen`, `byte löschen` | Kernel |
| 68 | 44 | GR_SET_TEXT_SPACE_PAD | `WBFixed` (3) | Kernel |
| 69 | 45 | GR_SET_TEXT_ATTR | `OldTextAttr` (19 oder 20, s. u.) | Kernel |
| 70 | 46 | GR_SET_FONT | `WBFixed größe`, `word FontID` (5) | Kernel |
| 71 | 47 | GR_SET_STRING_BOUNDS | – laut Convert (Größe ungeprüft) | Kernel |
| 72 | 48 | GR_CREATE_PALETTE | `word` | Kernel |
| 73 | 49 | GR_DESTROY_PALETTE | `word` | Kernel |
| 74 | 4A | GR_SET_PALETTE_ENTRY | `word` laut Convert (ungeprüft) | Kernel |
| 75 | 4B | GR_SET_PALETTE | `word` laut Convert (ungeprüft) | Kernel |
| 76 | 4C | GR_SET_BORDER_WIDTH | `word` | Kernel |
| 77 | 4D | GR_SET_BORDER_JOIN | `byte` | Kernel |
| 78 | 4E | GR_SET_LINE_COLOR_INDEX | `byte` | Kernel |
| 79 | 4F | GR_SET_CUSTOM_LINE_MASK | 8 Byte Muster | Kernel |
| 80 | 50 | GR_SET_AREA_COLOR_INDEX | `byte` | Kernel |
| 81 | 51 | GR_SET_CUSTOM_AREA_MASK | 8 Byte Muster | Kernel |
| 82 | 52 | GR_SET_TEXT_COLOR_INDEX | `byte` | Kernel |
| 83 | 53 | GR_SET_CUSTOM_TEXT_MASK | 8 Byte Muster | Kernel |
| 84 | 54 | GR_SET_CUSTOM_LINE_STYLE | `byte index`, `word n`, n × 2 Byte | Kernel |
| 85 | 55 | GR_SET_TRACK_KERN | `word` | Kernel |
| 86 | 56 | GR_INIT_DEFAULT_TRANSFORM | – | Kernel |
| 87 | 57 | GR_SET_DEFAULT_TRANSFORM | – | Kernel |
| 88 | 58 | GR_SET_CLIP_RECT | 10 Byte (Flags + Rechteck) | Kernel |
| 89 | 59 | GR_SET_DOC_CLIP_RECT | 10 Byte | Kernel |

## Strukturen

**OldTransMatrix (24 Byte):** `e11, e12, e21, e22, e31, e32`, jeweils `WWFixed`.
Die Translation e31/e32 ist nur 16+16 Bit breit, anders als `DWFixed` in 2.0.

**OldLineAttr (10 Byte, Opcode 56):** `byte colorFlag`, `byte R`, `byte G`, `byte B`,
`byte mapMode`, `byte mask` (Reihenfolge laut `PELineAttr`, das die beiden Bytes
tauscht; die Bits von mapMode werden dabei umgerechnet), `word width`,
`byte end`, `byte join`.

**OldAreaAttr (6 Byte, Opcode 62):** wie die ersten 6 Byte von OldLineAttr.

**OldTextAttr (Opcode 69 und in Text-Feldern), 20 Byte – am System bestätigt (20.11.2026):**
`byte colorFlag`, `RGB` (3), `byte mask`, [`byte mapMode`], `byte styleSet`,
`byte styleClear`, `byte modeSet`, `byte modeClear`, `WBFixed spacePad` (3),
`word font`, `WBFixed size` (3), `sword trackKern`.
Echte GeoDraw-Text-Felder haben fcount = 28 + Textlänge + 22, also Stil-Abschnitte
zu 2 + 20 Byte. `mapMode` ist vorhanden; `PETextAttr` in FreeGEOS überspringt es.
Beobachtet: Flag 00, Index 00, mask 19h (100 %), mapMode 01, styleClear FFh, modeSet 08h.
Gespeicherter Feldkopf beobachtet: next FFFFh, width 29h, textLeft 1, yPos 0Eh/20h/32h/44h
(Zeilenabstand 18), baseLine 0Dh.
In 2.0 kamen Schriftstärke, -breite und Muster dazu. Diese gibt es in 1.x nicht.

**Bitmap (Opcode 25/26):** Nach dem Slice-Größenwort folgt der Bitmap-Kopf:
`width`, `height`, `byte compact`, `byte type` (6 Byte). Bei „komplex“
(`type & 8`) kommen weitere Felder, die `PS.GEO` wie das spätere `CBitmap`
liest: +8 `numScans`, +0Ch `data`, +10h `xres`, +12h `yres`.
Weitere Slices werden über einen Callback nachgeladen.

**OldGDF_saved = 28 Byte:** Feld-Info 13 (next 2, flags 1, start 2, position 2,
width 2, spacePad 3, tab 1), OTab 4 (Position 2, Attribute 1, Anker 1),
areaLeft 2, adjustment 2, lineJust 1, textLeft 2, yPos 2, baseLine 2.
Startpunkt des Texts: x = position + textLeft + adjustment, y = yPos.

**Text-Feld (Opcode 33):** `word fcount` (Bytes ab hier bis einschließlich des
ersten Style-Runs), `OldGDF_saved` (Feld- und Zeileninfo, Aufbau in
`convertGString.def`), dann die Zeichen, dann `OldTFStyleRun` = `word count` +
`OldTextAttr`. Weitere Runs folgen.
Die Zeichenposition ergibt sich aus `OFI_position + textLeft + adjustment`, y = `yPos`.

## Folgen für den PDF-Treiber

1. **Zuerst nötig (deckt `PS.GEO` ab):** Linien (13, 14, 17–20), Rechtecke (15, 16, 40, 41),
   Polylinie/Polygon (35, 39, 45), Ellipsen (36, 46), Text (31, 32, 33), Bitmaps (25, 26).
2. **Lücken in `PS.GEO`, die der PDF-Treiber schließen sollte:**
   `GR_DRAW_CHAR`/`_CP` (29/30), `GR_DRAW_POINT` (23/24), `GR_DRAW_SPLINE` (38),
   `_PTR`/`_OPTR`-Varianten (27, 28, 34). Für diese Opcodes gibt PostScript heute nichts aus.
3. **Round Rect und Arc (21, 22, 37, 42–44)** markiert schon GeoWorks' eigener
   Konverter als Fehler. Bestätigt: Die zugehörigen Kernel-Einträge
   (322, 323, 334, 338–340) sind im 1.2-Kernel nur ein `RETF` (siehe `KERNEL12.md`).
   Der Treiber kann sie verwerfen und protokollieren.
4. Attribute liest man wie `PS.GEO` aus dem GState, statt Opcodes 47–89 selbst zu
   verfolgen. Dafür braucht es die Kernel-Getter. Ihre Ordinalnummern sind der nächste
   Rechercheschritt; die Kandidaten liegen bei den Einträgen, die `PS.GEO` importiert.
