; SPDX-License-Identifier: MIT
; K12 Test 3: GString abspielen.
; Teil A: 280 GrLoadGString, 283 GrDrawGString mit BP=4800h (bis zum
;         naechsten Zeichenbefehl), danach 204 GrCopyGString(CX=2000h)
;         in Block B, wie PS.GEO es macht. Zeigt Rueckgabe, DL und Kopie.
; Teil B: gleicher GString, nach jedem Stopp BP=2000h (ein Element
;         ausfuehren). Das Bild muss rechts unten erscheinen.
%include "k12head.inc"

run_tests:
 call o_reset
 P "K12 Test 3: GString abspielen (Stopp an Zeichenbefehlen)"
 call o_line
 add word [bp+FR_Y],4
 call rec_gstring
 call new_lmem_block
 mov [bp+FR_BLK2],bx
 mov [bp+FR_CHK2],ax

; ---------------- Teil A
 call o_reset
 P "Teil A: Stopp + GrCopyGString (wie PS.GEO)"
 call o_line
 call load_gs
 mov word [bp+FR_N],0
.a_loop:
 call play_stop            ; AX=Rueckgabe, FR_OP=DL
 mov [bp+FR_R1],ax
 call o_reset
 P " R="
 mov ax,[bp+FR_R1]
 call o_hex4
 P " DL="
 mov al,[bp+FR_OP]
 call o_hex2
 call is_done
 jc .a_end
 call copy_elem            ; Element nach Block B kopieren
 P "  Kopie: "
 call show_copy
 call o_line
 inc word [bp+FR_N]
 cmp word [bp+FR_N],12
 jb .a_loop
 call o_reset
 P " Abbruch nach 12 Stopps"
.a_end:
 call o_line
 call kill_gs

; ---------------- Teil B
 call o_reset
 P "Teil B: Stopp + ein Element ausfuehren (BP=2000h), Bild rechts unten"
 call o_line
 call load_gs
 mov word [bp+FR_N],0
 call o_reset
.b_loop:
 call play_stop
 mov [bp+FR_R1],ax
 mov al,[bp+FR_OP]
 call o_hex2
 mov al,'/'
 call o_chr
 mov ax,[bp+FR_R1]
 call o_hex2
 call o_sp
 call is_done
 jc .b_end
 push bp
 mov si,[bp+FR_GS]
 mov di,[bp+FR_WIN]
 xor ax,ax
 xor bx,bx
 xor cx,cx
 xor dx,dx
 mov bp,0x2000
 KCALL 283                 ; ein Element ausfuehren
 pop bp
 inc word [bp+FR_N]
 cmp word [bp+FR_N],12
 jb .b_loop
.b_end:
 call o_line
 call o_reset
 P " (Format: Opcode/Rueckgabe je Stopp)"
 call o_line
 call kill_gs

 mov bx,[bp+FR_BLK]
 KCALL 2
 mov bx,[bp+FR_BLK2]
 KCALL 2
 call o_reset
 P "Ende Test 3"
 call o_line
 ret

; 280 GrLoadGString(CL=0, BX=Block A, SI=Chunk A) -> SI
load_gs:
 mov bx,[bp+FR_BLK]
 mov si,[bp+FR_CHK]
 mov cl,0
 KCALL 280
 mov [bp+FR_GS],si
 call o_reset
 P " 280 GrLoadGString -> SI="
 mov ax,[bp+FR_GS]
 call o_hex4
 call o_line
 ret

; 279 GrDestroyGString(DI=SI=GString, DL=1) wie im Spooler
kill_gs:
 mov si,[bp+FR_GS]
 mov di,si
 mov dl,1
 KCALL 279
 ret

; 283 GrDrawGString mit BP=4800h. Aus: AX=Rueckgabe, FR_OP=DL
play_stop:
 push bp
 mov si,[bp+FR_GS]
 mov di,[bp+FR_WIN]
 xor ax,ax
 xor bx,bx
 xor cx,cx
 xor dx,dx
 mov bp,0x4800
 KCALL 283
 mov ax,bp
 pop bp
 mov [bp+FR_OP],dl
 ret

; CF=1 wenn fertig (Rueckgabe 0, 1 oder FFh, wie im Spooler)
is_done:
 mov ax,[bp+FR_R1]
 cmp ax,0
 je .y
 cmp ax,1
 je .y
 cmp ax,0xff
 je .y
 clc
 ret
.y:
 stc
 ret

; Wie PS.GEO Res. 9:531h: Chunk B leeren, 281 auf Chunk B, 204 kopieren, 279
copy_elem:
 mov bx,[bp+FR_BLK2]
 KCALL 5
 mov ds,ax
 mov bx,[bp+FR_BLK2]
 mov ax,[bp+FR_CHK2]
 xor cx,cx
 KCALL 28                  ; LMemReAlloc auf 0
 mov bx,[bp+FR_BLK2]
 KCALL 6
 push ss
 pop ds
 mov bx,[bp+FR_BLK2]
 mov ax,[bp+FR_CHK2]
 mov si,ax
 mov cl,0
 KCALL 281                 ; GrCreateGString -> DI
 mov [bp+FR_REC],di
 mov si,[bp+FR_GS]
 mov cx,0x2000
 KCALL 204                 ; GrCopyGString, ein Element
 mov di,[bp+FR_REC]
 mov si,[bp+FR_GS]
 mov dl,1
 KCALL 279
 push ss
 pop ds
 ret

; Kopiertes Element aus Chunk B anzeigen und erstes Byte mit DL vergleichen
show_copy:
 mov bx,[bp+FR_BLK2]
 KCALL 5
 mov es,ax
 mov [bp+FR_SEG],ax
 mov si,[bp+FR_CHK2]
 mov si,[es:si]
 mov cx,[es:si-2]
 mov al,[es:si]
 mov [bp+FR_E1],al
 cmp cx,14
 jbe .n
 mov cx,14
.n:
 call o_bytes
 mov bx,[bp+FR_BLK2]
 KCALL 6
 push ss
 pop es
 mov al,[bp+FR_E1]
 cmp al,[bp+FR_OP]
 jne .f
 P " =DL OK"
 ret
.f:
 P " !=DL"
 ret

%include "k12lib.inc"
G1_FINISH
