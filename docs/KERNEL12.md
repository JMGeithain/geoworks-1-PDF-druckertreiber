# Kernel-Einsprungnummern von GeoWorks 1.2

Referenz für den PDF-Druckertreiber. Stand 20.11.2026.
Ziel: deutsche GeoWorks-Ensemble-1.2-Installation, `KERNEL.EXE`
(sha256 `c2cc8952cdb01d0f…`), `SYSTEM\KLIB.GEO` (sha256 `f7465303446b6776…`).

Vollständige Liste maschinenlesbar in `data/kernel12.json` (169 Einträge),
als Konstanten in `include/kern12.inc` / `include/kern12.h` (Präfix `K12_`).

## Wie die Namen zustande kommen

In GeoWorks 1.2 stehen keine Funktionsnamen in den Binaries. Die Namen wurden
aus fünf voneinander unabhängigen Quellen erschlossen:

1. **Einsprungtabelle des Kernels.** `KERNEL.EXE` enthält ab Image-Offset `0deb0h`
   (Segment `0deb`) eine Tabelle von 577 Near-Offsets. Der Lader liest sie beim
   Auflösen von Kernel-Relocations über `cs:[2147h]`. Damit führt jede Nummer zu
   einer Code-Adresse. Viele Grafik-Einträge sind dort nur Stummel, die mit
   `mov ax,N` in die Exportnummer N von `KLIB.GEO` springen.
2. **Abspieltabelle in `KLIB.GEO`** (Ressource 17): 90 Far-Pointer auf
   Kernel-Einträge, einer je GString-Opcode. Zusammen mit der Opcode-Liste aus
   FreeGEOS (`Library/Convert/convertGStringTables.asm`) ergibt das 79 Namen
   direkt (Einträge 202, 210, 301–378). Fast alle stimmen mit den im SDK beobachteten
   Nummern überein; die einzige Ausnahme ist unten erklärt.
3. **DOS-Funktionsnummern.** Die Datei-Einträge rufen `INT 21h` mit
   charakteristischem AH auf (3Dh öffnen, 3Eh schließen, 3Fh lesen …).
4. **GState-Felder.** Jeder Getter liest dasselbe Feld, das der (über Punkt 2
   sicher benannte) Setter schreibt, z. B. 271 liest das Wort `[5Eh]`, das
   `GrSetLineWidth` (349) schreibt.
5. **Aufrufkontext** in `PS.GEO`, `PSCRIPT.GEO` und `SPOOL.GEO` (welche Register
   vorher gesetzt und danach benutzt werden), plus die Reihenfolge der
   Exportliste von 2.0 (`geos.gp`), die die 1.x-Reihenfolge abschnittsweise
   beibehalten hat.

**Sicherheitsstufen:** *sicher* = Quellen 1–4 oder vom SDK beobachtet und am Code
bestätigt; *wahrscheinlich* = nur Aufrufkontext und Reihenfolge;
*Funktion bekannt* = Zweck klar, 1.x-Name offen.
Ergebnis: 126 sicher, 24 am System getestet, 15 wahrscheinlich, 1 offen, 3 nur Funktion.

## Korrektur am SDK-Profil

`pcgeos-1-sdk-v2` führt **324 als `GrDrawEllipse`**. Das stimmt nicht:
324 setzt Start = Ende und ist laut KLIB-Tabelle `GrDrawPoint` (Opcode 23).
`GrDrawEllipse` ist **333** (Opcode 36) und teilt sich den Code mit
`GrFillEllipse` (342). Ein Programm, das 324 für Ellipsen benutzt, zeichnet nur
einen Punkt. Das ist dem SDK-Test mit dem Rechteck −100,−100,0,0 vermutlich
nicht aufgefallen, weil es außerhalb des Fensters liegt.

## Weitere Befunde

* Round-Rect und Arc (322, 323, 334, 338–340) sind im 1.2-Kernel nur ein `RETF`.
  Diese Befehle gibt es in 1.2 also zwar als Opcode, sie zeichnen aber nichts.
* 1.x-Namen, die in 2.0 anders heißen: `GrSetDrawMode` → `GrSetMixMode`,
  `GrSetDocClipRect` → `GrSetWinClipRect`, `GrEndString` → `GrEndGString`,
  `GrSetStringBounds` → `GrSetGStringBounds`. `GrSetBorderWidth`/`-Join`
  (373/374) und ihre Getter (290/291) gibt es in 2.0 nicht mehr.
* `KERNEL.OLD` (1994) ist ein anderer Build mit ebenfalls 577 Einträgen und
  verschobenen Adressen. Die Nummern gelten für beide, die Adressen nur für
  `KERNEL.EXE`.
* `KLIB.GEO` dieser Installation hat einen anderen Hash als im SDK-Zielprofil.
  Eine Identitätsprüfung des SDK würde hier also anschlagen.

## Die 60 Einträge, die `PS.GEO` braucht

| Nr. | Name 1.x | Name 2.0 | Sicherheit | Begründung |
|---:|---|---|---|---|
| 0 | MemAlloc | = | sicher | pcgeos-1-sdk-v2 (beobachtet), Code-Form passt |
| 2 | MemFree | = | sicher | pcgeos-1-sdk-v2 (beobachtet), Code-Form passt |
| 5 | MemLock | = | sicher | pcgeos-1-sdk-v2 (beobachtet), Code-Form passt |
| 6 | MemUnlock | = | sicher | pcgeos-1-sdk-v2 (beobachtet), Code-Form passt |
| 24 | LMemInitHeap | = | sicher (getestet) | PS.GEO: frischer Block, AX=Typ, CX=Handles, DX=Kopfgroesse; Test K12 (GeoWorks 1.28, 20.11.2026): OK |
| 25 | LMemAlloc | = | sicher (getestet) | CX=Groesse -> AX=Chunk (PS.GEO, 6 Aufrufe nach Init); Test K12 (GeoWorks 1.28, 20.11.2026): OK |
| 27 | LMemFree | = | sicher (getestet) | AX=Chunk, gibt Chunk frei; Test K12 (GeoWorks 1.28, 20.11.2026): ohne Absturz |
| 28 | LMemReAlloc | = | sicher (getestet) | AX=Chunk, CX=neue Groesse; Test K12 (GeoWorks 1.28, 20.11.2026): Inhalt bleibt erhalten, Laengenwort = Groesse+2 OK |
| 52 | FileOpen | = | sicher | pcgeos-1-sdk-v2 (beobachtet), Code-Form passt; DOS 3Dh |
| 54 | FileClose | = | sicher | ruft DOS INT 21h AH=3Eh |
| 58 | FileRead | = | sicher | ruft DOS INT 21h AH=3Fh |
| 59 | FileWrite | = | sicher | ruft DOS INT 21h AH=40h |
| 60 | FilePos | = | sicher | ruft DOS INT 21h AH=42h |
| 73 | FilePushDir | = | wahrscheinlich | Paar mit 74 um FileCreate/FileOpen in PSCRIPT/SPOOL |
| 74 | FilePopDir | = | wahrscheinlich | s. 73 |
| 76 | FileSetStandardPath | = | wahrscheinlich | AX=Standardpfad (3, 6) vor FileOpen/FileCreate |
| 91 | FileSize | = | wahrscheinlich | BX=Datei, Ergebnis als MemAlloc-Groesse |
| 147 | (Ressource sperren) | – | Funktion bekannt | Handle-Lock mit Nachladen; BX=Ressourcen-Handle -> AX=Segment, freigeben mit MemUnlock |
| 187 | TimerGetDateAndTime | = | sicher (getestet) | liefert AX..DX aus der Kernel-Uhr; PS.GEO Datumszeile; Test K12 (GeoWorks 1.28, 20.11.2026): AX=Jahr, BL=Monat, BH=Tag, CL=Wochentag, CH=Std, DL=Min, DH=Sek OK |
| 198 | GrFontMetrics | = | wahrscheinlich | SI=Metrik-Index -> DX.AH; Test K12 (GeoWorks 1.28, 20.11.2026): SI=2/6/12h/14h -> 6, 10, 11, 1 (plausibel, Bedeutung der Indizes offen) |
| 204 | GrCopyGString | = | sicher (getestet) | zwischen GrCreateGString und GrDestroyGString, CX=2000h; KLIB #43; Test K12 (GeoWorks 1.28, 20.11.2026): SI=Quelle, DI=Ziel-GState, CX=2000h kopiert das anstehende Element und rueckt weiter OK |
| 208 | GrCallFontDriverID | = | wahrscheinlich | AX=FontID, DI=Treiberfunktion |
| 211 | GrCreateState | = | sicher | pcgeos-1-sdk-v2 (beobachtet), Code-Form passt |
| 212 | GrDestroyState | = | sicher | pcgeos-1-sdk-v2 (beobachtet), Code-Form passt |
| 221 | (Schrift sperren) | – | Funktion bekannt | DS=GState -> AX=Segment des Font-Blocks |
| 222 | (Schrift freigeben) | – | Funktion bekannt | Gegenstueck zu 221 |
| 223 | GrTextWidthWBFixed | = | sicher (getestet) | nach GrSetTextAttr, Ergebnis DX.AH; Test K12 (GeoWorks 1.28, 20.11.2026): DI=GState, DS:SI=Text, CX=Anzahl -> DX.AH Breite; Strich am Textende OK |
| 227 | GrMulWWFixed | = | sicher (getestet) | DX.CX * BX.AX, dpi-Umrechnung; Test K12 (GeoWorks 1.28, 20.11.2026): 3.5*2=7.0 OK |
| 228 | GrSDivWWFixed | = | sicher (getestet) | Division DX.CX / BX.AX; Test K12 (GeoWorks 1.28, 20.11.2026): -7/2=-3.5 OK |
| 229 | GrUDivWWFixed | = | sicher (getestet) | Division durch Aufloesung vor Mul 72; Test K12 (GeoWorks 1.28, 20.11.2026): 9/4=2.25 OK |
| 247 | GrMapColorIndex | = | sicher (getestet) | AL/AH=Palettenindex, DI=0; Test K12 (GeoWorks 1.28, 20.11.2026): AH=Index -> AL=R, BL=G, BH=B (Index 4 = AA 00 00) OK |
| 259 | GrGetLineColor | = | sicher | liest GState-Feld Linien-Attributblock 0x4c |
| 260 | GrGetAreaColor | = | sicher | liest GState-Feld Flaechen-Block 0x7b |
| 261 | GrGetTextColor | = | sicher | liest GState-Feld Text-Block 0x91 |
| 262 | GrGetLineMask | = | sicher | liest GState-Feld Block 0x4c |
| 263 | GrGetAreaMask | = | sicher | liest GState-Feld Block 0x7b |
| 268 | GrGetTextSpacePad | = | sicher | liest GState-Feld 0xac/0xab wie GrSetTextSpacePad (364) |
| 269 | GrGetTextStyle | = | sicher | liest GState-Feld 0x8f wie GrSetTextStyle (362) |
| 270 | GrGetTextMode | = | sicher | liest GState-Feld 0x90 wie GrSetTextMode (363) |
| 271 | GrGetLineWidth | = | sicher (getestet) | liest GState-Feld Wort 0x5e wie GrSetLineWidth (349); Test K12 (GeoWorks 1.28, 20.11.2026): Set/Get 3 OK |
| 272 | GrGetLineEnd | = | sicher | liest GState-Feld 0x5d wie GrSetLineEnd (351) |
| 273 | GrGetLineJoin | = | sicher | liest GState-Feld 0x5c wie GrSetLineJoin (350) |
| 274 | GrGetMiterLimit | = | sicher | liest GState-Feld 0x62 + Hilfsroutine 66E0h wie GrSetMiterLimit (353) |
| 275 | GrGetCurPos | = | sicher (getestet) | GrGetInfo mit festem Typ 4; Test K12 (GeoWorks 1.28, 20.11.2026): MoveTo 50,60 -> AX,BX OK |
| 279 | GrDestroyGString | = | sicher (getestet) | DI=GString, DL=1; KLIB #44; Test K12 (GeoWorks 1.28, 20.11.2026): DI=GState/GString, DL=1 behaelt Daten OK |
| 280 | GrLoadGString | = | sicher (getestet) | BX=Datei, CL=Typ -> SI=GString (Spooler); Test K12 (GeoWorks 1.28, 20.11.2026): CL=0, BX=Block, SI=Chunk -> SI=GString OK |
| 281 | GrCreateGString | = | sicher (getestet) | wie 280 mit CH=1; KLIB #52; Test K12 (GeoWorks 1.28, 20.11.2026): CL=0, BX=Block, AX=SI=Chunk -> DI (Aufzeichnungs-GState) OK |
| 283 | GrDrawGString | = | sicher (getestet) | GrMoveTo(AX,BX), dann KLIB #42; BP=Steuerflags, DX=Stopp-Opcode; Test K12 (GeoWorks 1.28, 20.11.2026): BP=4800h: Stopp vor jedem Zeichenbefehl, BP=4 zurueck, DL=Opcode; Ende BP=FFh, DL=0. BP=2000h fuehrt ein Element aus. Attribute wirken auf den Ziel-GState. OK |
| 286 | GrGetTransform | = | sicher (getestet) | DS:SI=24-Byte-Puffer (TransMatrix); Test K12 (GeoWorks 1.28, 20.11.2026): DS:SI=24 Byte, 6x WWFixed (frac,int); ApplyScale DX.CX=x, BX.AX=y OK |
| 293 | GrGetFont | = | sicher (getestet) | liest GState-Feld 0xb0/0xb2/0xb3 wie GrSetFont (366); Test K12 (GeoWorks 1.28, 20.11.2026): CX=FontID 0202, DX.AH=Groesse 10 pt |
| 295 | GrGetLineStyle | = | sicher (getestet) | GState 0x5b neben Join/End/Width; Test K12 (GeoWorks 1.28, 20.11.2026): Set/Get 1 OK |
| 297 | GrCheckFontAvail | = | offen (Test widerspruechlich) | CX=FontID, DL=Flags -> CX=0 wenn fehlt; Test K12 (GeoWorks 1.28, 20.11.2026): eigene ID 0202 und 7777h liefern beide 0. Vermutung: DL=1 fragt nur Outline-Schriften ab, Berkeley ist Bitmap. Nachtest mit DL=0 und Outline-Schrift noetig |
| 305 | GrSaveState | = | sicher | KLIB.GEO Abspieltabelle, Opcode 4 (GR_SAVE_STATE) |
| 306 | GrRestoreState | = | sicher | KLIB.GEO Abspieltabelle, Opcode 5 (GR_RESTORE_STATE) |
| 309 | GrApplyScale | = | sicher | KLIB.GEO Abspieltabelle, Opcode 8 (GR_APPLY_SCALE) |
| 313 | GrSetNullTransform | = | sicher | KLIB.GEO Abspieltabelle, Opcode 12 (GR_SET_NULL_TRANSFORM) |
| 345 | GrMoveTo | = | sicher | KLIB.GEO Abspieltabelle, Opcode 49 (GR_MOVE_TO) |
| 362 | GrSetTextStyle | = | sicher | KLIB.GEO Abspieltabelle, Opcode 66 (GR_SET_TEXT_STYLE) |
| 365 | GrSetTextAttr | = | sicher | KLIB.GEO Abspieltabelle, Opcode 69 (GR_SET_TEXT_ATTR) |
| 366 | GrSetFont | = | sicher (getestet) | KLIB.GEO Abspieltabelle, Opcode 70 (GR_SET_FONT); Test K12 (GeoWorks 1.28, 20.11.2026): Element 46: WBFixed Groesse, FontID OK |

## Laufzeittests (K12 Test 1–3, 20.11.2026)

Alle Tests liefen in GeoWorks 1.28 ohne Absturz. Bestätigt sind die mit „sicher (getestet)“ markierten Einträge, inklusive Registerbelegung. Offen bleibt nur 297 `GrCheckFontAvail`.

## Ursprüngliche Testplanung

Alle *wahrscheinlich*-Einträge, die der PDF-Treiber tatsächlich aufruft, bekommen
ein kleines Testprogramm, das das Ergebnis auf den Bildschirm schreibt. Am
wichtigsten für den Treiber: 283/284 (`GrDrawGString` mit Stopp-Flags),
281/282, 286 (`GrGetTransform`), 223 (Textbreite), 297 (`GrCheckFontAvail`)
und die LMem-Aufrufe 24–28.

## Nachvollziehen

```
python3 tools/extract_kernel12.py <GEOWORKS-Verzeichnis> convertGStringTables.asm
```
liefert Einsprungtabelle, KLIB-Stummel und Opcode-Tabelle als JSON.
