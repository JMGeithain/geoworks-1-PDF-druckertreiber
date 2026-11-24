; SPDX-License-Identifier: MIT
; Gemeinsamer Kopf der K12-Testprogramme: Methoden wie im SDK-Beispiel.
%include "ensemble12.inc"
%define PROCESS_CLASS 0x40

; Kernel-Nummern siehe doc/KERNEL12.md. Alle Routinen sind NEAR.
;
; Rahmen (SS:BP), wird in view_exposed angelegt:
FR_BUF   equ 0            ; Zeilenpuffer
FR_BUFSZ equ 100
FR_LEN   equ 100
FR_Y     equ 102
FR_WIN   equ 104          ; GState des Fensters
FR_FONT  equ 106          ; FontID aus GrGetFont
FR_BLK   equ 108          ; LMem-Block A
FR_CHK   equ 110          ; Chunk in A
FR_BLK2  equ 112          ; LMem-Block B (Kopierpuffer)
FR_CHK2  equ 114
FR_REC   equ 116          ; GState beim Aufzeichnen
FR_GS    equ 118          ; geladener GString / Hilfs-GState
FR_N     equ 120          ; Schleifenzaehler
FR_R1    equ 122
FR_R2    equ 124
FR_E1    equ 126
FR_E2    equ 128
FR_SEG   equ 130
FR_END   equ 132
FR_OP    equ 134
FR_TM    equ 136          ; 24 Byte TransMatrix
FR_STR   equ 160          ; 16 Byte Textpuffer
FRAME    equ 192

LINE_H   equ 12

; P "text" haengt Text aus dem Codesegment an die aktuelle Zeile an.
%macro P 1+
 call o_inl
 db %1,0
%endmacro



G1_METHOD 0x0a01, app_open
G1_METHOD 0x0a03, app_close
G1_METHOD 0x0004, meta_attach
G1_METHOD 0x0057, view_closed
G1_METHOD 0x0049, view_exposed

app_open:
 push es
 pop ds
 mov di,PROCESS_CLASS
 KCALL 489
 retf
app_close:
 xor cx,cx
 retf
meta_attach:
 mov di,PROCESS_CLASS
 KCALL 489
 retf
view_closed:
 mov di,PROCESS_CLASS
 KCALL 489
 retf
view_exposed:
 jmp k12_expose
