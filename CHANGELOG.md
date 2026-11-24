# Änderungen

## 1.2a – 21.11.2026
- Absturz „bad handle passed to system“ beim Drucken von GeoWrite-Texten mit Grafiken
  behoben: `GrDestroyGString` erwartet in 1.x das GString-Handle in DI.

## 1.2 – 21.11.2026
- In GeoWrite eingefügte Grafiken werden gedruckt: GStrings, die in Textfeldern
  eingebettet sind, werden geladen und mit eigenem GState abgespielt.
- Bitmaps aus mehreren Scheiben enden nach der letzten Scheibe.

## 1.1 – 20.11.2026
- Flächenmuster 1–24 (Gitter, Linien, Ziegel …) als PDF-Kachelmuster.
- Querformat: `/Rotate` je nach Seitentransformation.
- Alle Ressourcen in einem gemeinsamen Objekt 3.

## 1.0 – 20.11.2026
- Keine Diagnose-Kommentare mehr im PDF.
- Glyphendaten werden stückweise gelesen (Zeichen beliebiger Größe).
- DGROUP verkleinert; Anleitung LIESMICH.TXT.

## Meilensteine M0–M8 – 20.11.2026
- M8: Umrissschriften aus `FONT\*.FNT` als Type3-Schriften eingebettet,
  fett und kursiv nachgebildet, Text durchsuchbar.
- M7: Maske 0 % = durchsichtig.
- M6: Bitmaps; Symbol- und Dingbats-Schriften; Tabulatoren nicht mehr als Zeichen
  ausgegeben; Spooler-Patch .PS → .PDF.
- M5: Graumuster als Aufhellung.
- M4: Hoch- und Tiefstellung; Grundlinie nach `TM_DRAW_BASE`.
- M3: Unter- und Durchstreichen; Zeilenbreite wie in GEOS.
- M2: Schriftzuordnung nach FontID; fett und kursiv.
- M1: Formen mit Objekt-Transformationen, Ellipsen, Text und Textfelder.
- M0: erster ladbarer Treiber.
