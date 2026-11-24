; SPDX-License-Identifier: MIT
; K12 Test 1: Mathe, Getter, Schrift, Datum, LMem.
; Prueft die "wahrscheinlich"-Eintraege aus doc/KERNEL12.md, die der
; PDF-Treiber braucht. Keine GString-Aufrufe (siehe Test 2 und 3).
%include "k12head.inc"

run_tests:
 call o_reset
 P "K12 Test 1: Mathe, Getter, Schrift, LMem (alle Werte hex)"
 call o_line
 add word [bp+FR_Y],4

; --- 227 GrMulWWFixed: 3.5 * 2.0 = 7.0
 mov dx,3
 mov cx,0x8000
 mov bx,2
 xor ax,ax
 KCALL 227
 mov [bp+FR_R1],dx
 mov [bp+FR_R2],cx
 mov word [bp+FR_E1],7
 mov word [bp+FR_E2],0
 call o_reset
 P "227 GrMulWWFixed   3.5 * 2    = "
 call show_fx

; --- 228 GrSDivWWFixed: -7.0 / 2.0 = -3.5 (FFFC.8000)
 mov dx,0xfff9
 xor cx,cx
 mov bx,2
 xor ax,ax
 KCALL 228
 mov [bp+FR_R1],dx
 mov [bp+FR_R2],cx
 mov word [bp+FR_E1],0xfffc
 mov word [bp+FR_E2],0x8000
 call o_reset
 P "228 GrSDivWWFixed  -7 / 2     = "
 call show_fx

; --- 229 GrUDivWWFixed: 9.0 / 4.0 = 2.25
 mov dx,9
 xor cx,cx
 mov bx,4
 xor ax,ax
 KCALL 229
 mov [bp+FR_R1],dx
 mov [bp+FR_R2],cx
 mov word [bp+FR_E1],2
 mov word [bp+FR_E2],0x4000
 call o_reset
 P "229 GrUDivWWFixed  9 / 4      = "
 call show_fx

; --- 293 GrGetFont (Fenster), schon in k12_expose gelesen
 mov di,[bp+FR_WIN]
 KCALL 293
 mov [bp+FR_R1],cx
 mov [bp+FR_R2],dx
 mov [bp+FR_E1],ax
 call o_reset
 P "293 GrGetFont      FontID="
 mov ax,[bp+FR_R1]
 call o_hex4
 P " Groesse="
 mov ax,[bp+FR_R2]
 call o_hex4
 mov al,'.'
 call o_chr
 mov ax,[bp+FR_E1]
 mov al,ah
 call o_hex2
 P "  (Pruefen: Standardschrift, Groesse plausibel?)"
 call o_line

; --- 297 GrCheckFontAvail
 mov cx,[bp+FR_FONT]
 mov dl,1
 KCALL 297
 mov [bp+FR_R1],cx
 mov ax,[bp+FR_FONT]
 mov [bp+FR_E1],ax
 call o_reset
 P "297 GrCheckFontAvail(eigene ID) = "
 call show_w1
 mov cx,0x7777
 mov dl,1
 KCALL 297
 mov [bp+FR_R1],cx
 mov word [bp+FR_E1],0
 call o_reset
 P "297 GrCheckFontAvail(7777h)     = "
 call show_w1

; --- Hilfs-GState ohne Fenster
 xor di,di
 KCALL 211
 mov [bp+FR_GS],di

; 349 GrSetLineWidth(AX=3) -> 271 GrGetLineWidth
 mov di,[bp+FR_GS]
 mov ax,3
 KCALL 349
 mov di,[bp+FR_GS]
 xor ax,ax
 KCALL 271
 mov [bp+FR_R1],ax
 mov word [bp+FR_E1],3
 call o_reset
 P "271 GrGetLineWidth nach Set 3   = "
 call show_w1

; 343 GrSetDrawMode(AL=4) -> 258 GrGetMixMode
 mov di,[bp+FR_GS]
 mov ax,4
 KCALL 343
 mov di,[bp+FR_GS]
 xor ax,ax
 KCALL 258
 xor ah,ah
 mov [bp+FR_R1],ax
 mov word [bp+FR_E1],4
 call o_reset
 P "258 GrGetMixMode nach Set 4     = "
 call show_w1

; 354 GrSetLineStyle(AL=1,BL=0) -> 295 GrGetLineStyle
 mov di,[bp+FR_GS]
 mov ax,1
 xor bx,bx
 KCALL 354
 mov di,[bp+FR_GS]
 xor ax,ax
 KCALL 295
 xor ah,ah
 mov [bp+FR_R1],ax
 mov word [bp+FR_E1],1
 call o_reset
 P "295 GrGetLineStyle nach Set 1   = "
 call show_w1

; 345 GrMoveTo(50,60) -> 275 GrGetCurPos
 mov di,[bp+FR_GS]
 mov ax,50
 mov bx,60
 KCALL 345
 mov di,[bp+FR_GS]
 xor ax,ax
 xor bx,bx
 KCALL 275
 mov [bp+FR_R1],ax
 mov [bp+FR_R2],bx
 mov word [bp+FR_E1],50
 mov word [bp+FR_E2],60
 call o_reset
 P "275 GrGetCurPos nach MoveTo 50,60 = "
 call show_w2

; 313 GrSetNullTransform, 309 GrApplyScale(2,3) -> 286 GrGetTransform
 mov di,[bp+FR_GS]
 KCALL 313
 mov di,[bp+FR_GS]
 mov dx,2
 xor cx,cx
 mov bx,3
 xor ax,ax
 KCALL 309
 mov di,[bp+FR_GS]
 push ds
 push ss
 pop ds
 lea si,[bp+FR_TM]
 KCALL 286
 pop ds
 mov ax,[bp+FR_TM+2]
 mov [bp+FR_R1],ax
 mov ax,[bp+FR_TM+14]
 mov [bp+FR_R2],ax
 mov word [bp+FR_E1],2
 mov word [bp+FR_E2],3
 call o_reset
 P "286 GrGetTransform e11,e22 (int) = "
 call show_w2
 call o_reset
 P "    Matrix roh: "
 xor si,si
.tm:
 mov ax,[bp+si+FR_TM]
 call o_hex4
 call o_sp
 add si,2
 cmp si,24
 jb .tm
 call o_line

 mov di,[bp+FR_GS]
 KCALL 212                 ; GrDestroyState

; --- 247 GrMapColorIndex(AH=4 rot, DI=0)
 mov ah,4
 xor al,al
 xor di,di
 KCALL 247
 mov [bp+FR_R1],ax
 mov [bp+FR_R2],bx
 call o_reset
 P "247 GrMapColorIndex(4) AX="
 mov ax,[bp+FR_R1]
 call o_hex4
 P " BX="
 mov ax,[bp+FR_R2]
 call o_hex4
 P "  (rot = AA 00 00 irgendwo?)"
 call o_line

; --- 198 GrFontMetrics, SI = 2, 6, 12h, 14h
 call o_reset
 P "198 GrFontMetrics SI=2/6/12/14: "
 mov word [bp+FR_N],0
.fm:
 mov bx,[bp+FR_N]
 mov si,[cs:bx+fm_idx]
 mov di,[bp+FR_WIN]
 xor ax,ax
 xor dx,dx
 KCALL 198
 mov [bp+FR_R1],ax
 mov ax,dx
 call o_hex4
 mov al,'.'
 call o_chr
 mov ax,[bp+FR_R1]
 mov al,ah
 call o_hex2
 call o_sp
 add word [bp+FR_N],2
 cmp word [bp+FR_N],8
 jb .fm
 call o_line

; --- 223 Textbreite: Text zeichnen und senkrechten Strich an Ende setzen
 mov di,[bp+FR_WIN]
 push ds
 push cs
 pop ds
 mov si,str_hw
 mov cx,10
 xor ax,ax
 xor dx,dx
 KCALL 223
 pop ds
 mov [bp+FR_R1],dx
 mov [bp+FR_R2],ax
 call o_reset
 P "223 Textbreite('Hallo Welt') = "
 mov ax,[bp+FR_R1]
 call o_hex4
 mov al,'.'
 call o_chr
 mov ax,[bp+FR_R2]
 mov al,ah
 call o_hex2
 P "  Strich muss am Textende stehen:"
 call o_line
 mov di,[bp+FR_WIN]
 push ds
 push cs
 pop ds
 mov si,str_hw
 mov cx,10
 mov ax,40
 mov bx,[bp+FR_Y]
 KCALL 330                 ; GrDrawText bei x=40
 pop ds
 mov ax,40
 add ax,[bp+FR_R1]
 mov cx,ax
 mov bx,[bp+FR_Y]
 mov dx,bx
 add dx,LINE_H
 mov di,[bp+FR_WIN]
 KCALL 314                 ; GrDrawLine senkrecht
 mov ax,38
 mov cx,38
 mov bx,[bp+FR_Y]
 mov dx,bx
 add dx,LINE_H
 mov di,[bp+FR_WIN]
 KCALL 314                 ; Strich am Textanfang
 add word [bp+FR_Y],LINE_H+4

; --- 187 TimerGetDateAndTime
 KCALL 187
 mov [bp+FR_R1],ax
 mov [bp+FR_R2],bx
 mov [bp+FR_E1],cx
 mov [bp+FR_E2],dx
 call o_reset
 P "187 Datum/Zeit roh AX="
 mov ax,[bp+FR_R1]
 call o_hex4
 P " BX="
 mov ax,[bp+FR_R2]
 call o_hex4
 P " CX="
 mov ax,[bp+FR_E1]
 call o_hex4
 P " DX="
 mov ax,[bp+FR_E2]
 call o_hex4
 call o_line
 call o_reset
 P "    (Erwartung: AX=Jahr, BL=Monat, BH=Tag, CH=Stunde, DL=Minute)"
 call o_line

; --- LMem 24/25/28/27
 call new_lmem_block
 mov [bp+FR_BLK],bx
 mov [bp+FR_CHK],ax
 call o_reset
 P "24/25 LMemInitHeap+LMemAlloc: Block="
 mov ax,[bp+FR_BLK]
 call o_hex4
 P " Chunk="
 mov ax,[bp+FR_CHK]
 call o_hex4
 call o_line
 mov bx,[bp+FR_BLK]
 KCALL 5                   ; MemLock
 mov ds,ax
 mov bx,[bp+FR_BLK]
 mov ax,[bp+FR_CHK]
 mov cx,10
 KCALL 28                  ; LMemReAlloc auf 10
 mov si,[bp+FR_CHK]
 mov si,[si]
 mov word [si],0x1234
 mov word [si+8],0xabcd
 mov bx,[bp+FR_BLK]
 mov ax,[bp+FR_CHK]
 mov cx,40
 KCALL 28                  ; LMemReAlloc auf 40
 mov si,[bp+FR_CHK]
 mov si,[si]
 mov ax,[si]
 mov [bp+FR_R1],ax
 mov ax,[si+8]
 mov [bp+FR_R2],ax
 mov ax,[si-2]
 mov [bp+FR_N],ax
 push ss
 pop ds
 mov word [bp+FR_E1],0x1234
 mov word [bp+FR_E2],0xabcd
 call o_reset
 P "28 LMemReAlloc 10->40, Inhalt = "
 call show_w2
 call o_reset
 P "   Laengenwort nach ReAlloc = "
 mov ax,[bp+FR_N]
 call o_hex4
 P "  (erw. 28h oder 2Ah)"
 call o_line
 mov bx,[bp+FR_BLK]
 KCALL 5
 mov ds,ax
 mov bx,[bp+FR_BLK]
 mov ax,[bp+FR_CHK]
 KCALL 27                  ; LMemFree
 push ss
 pop ds
 mov bx,[bp+FR_BLK]
 KCALL 6                   ; MemUnlock
 mov bx,[bp+FR_BLK]
 KCALL 2                   ; MemFree
 call o_reset
 P "27 LMemFree, MemUnlock, MemFree ausgefuehrt - Ende Test 1"
 call o_line
 ret

fm_idx: dw 2,6,0x12,0x14
str_hw: db 'Hallo Welt'

%include "k12lib.inc"
G1_FINISH
