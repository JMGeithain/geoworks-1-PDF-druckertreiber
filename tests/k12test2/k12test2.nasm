; SPDX-License-Identifier: MIT
; K12 Test 2: GString in einen LMem-Chunk aufzeichnen und Element fuer
; Element anzeigen. Prueft 24/25/281/301/279 und die Byte-Layouts aus
; doc/GSTRING12.md (u. a. GR_SET_FONT und GR_DRAW_CHAR).
%include "k12head.inc"

run_tests:
 call o_reset
 P "K12 Test 2: GString aufzeichnen und Bytes anzeigen (hex)"
 call o_line
 call o_reset
 P "Aufgezeichnet: LineColor(1) Line FillColor(4) FillRect Font(14pt) Text'Ab' Char'Z' Ellipse"
 call o_line
 call o_reset
 P "erwartete Opcodes: 4E/32 0D 50/3B 28 46 1F 1D 24 00"
 call o_line
 add word [bp+FR_Y],4
 call rec_gstring
 call dump_gstring
 mov bx,[bp+FR_BLK]
 KCALL 2                   ; MemFree
 call o_reset
 P "Ende Test 2 (Block freigegeben). FontID war "
 mov ax,[bp+FR_FONT]
 call o_hex4
 call o_line
 ret

%include "k12lib.inc"
G1_FINISH
