; SPDX-License-Identifier: MIT
; Routinen der K12-Testprogramme (Definitionen in k12head.inc).

; ---------------------------------------------------------------- Ausgabe
o_reset:
 mov word [bp+FR_LEN],0
 ret

o_chr:                     ; AL anhaengen
 push di
 mov di,[bp+FR_LEN]
 cmp di,FR_BUFSZ
 jae .x
 mov [bp+di+FR_BUF],al
 inc word [bp+FR_LEN]
.x:
 pop di
 ret

o_inl:                     ; Inline-String nach dem CALL
 pop si
.l:
 mov al,[cs:si]
 inc si
 or al,al
 jz .d
 call o_chr
 jmp .l
.d:
 push si
 ret

o_hex2:                    ; AL als 2 Hexziffern
 push ax
 push cx
 mov ah,al
 mov cl,4
 shr al,cl
 call .nib
 mov al,ah
 and al,0x0f
 call .nib
 pop cx
 pop ax
 ret
.nib:
 add al,'0'
 cmp al,'9'
 jbe .o
 add al,7
.o:
 jmp o_chr

o_hex4:                    ; AX als 4 Hexziffern
 push ax
 xchg al,ah
 call o_hex2
 pop ax
 push ax
 call o_hex2
 pop ax
 ret

o_sp:
 mov al,' '
 jmp o_chr

o_line:                    ; Zeile zeichnen, naechste Zeile
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 mov cx,[bp+FR_LEN]
 jcxz .skip
 push ds
 mov ax,ss
 mov ds,ax
 lea si,[bp+FR_BUF]
 mov di,[bp+FR_WIN]
 mov ax,6
 mov bx,[bp+FR_Y]
 KCALL 330                 ; GrDrawText
 pop ds
.skip:
 add word [bp+FR_Y],LINE_H
 mov word [bp+FR_LEN],0
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 ret

o_okfail:                  ; ZF gesetzt -> OK, sonst FEHLER, dann Zeile
 jne .f
 P "  OK"
 jmp o_line
.f:
 P "  FEHLER"
 jmp o_line

; R1.R2 ausgeben, mit E1.E2 vergleichen
show_fx:
 mov ax,[bp+FR_R1]
 call o_hex4
 mov al,'.'
 call o_chr
 mov ax,[bp+FR_R2]
 call o_hex4
 P "  erw. "
 mov ax,[bp+FR_E1]
 call o_hex4
 mov al,'.'
 call o_chr
 mov ax,[bp+FR_E2]
 call o_hex4
 mov ax,[bp+FR_R1]
 cmp ax,[bp+FR_E1]
 jne .n
 mov ax,[bp+FR_R2]
 cmp ax,[bp+FR_E2]
.n:
 jmp o_okfail

; R1 (und R2) ausgeben, mit E1 (und E2) vergleichen
show_w1:
 mov ax,[bp+FR_R1]
 call o_hex4
 P "  erw. "
 mov ax,[bp+FR_E1]
 call o_hex4
 mov ax,[bp+FR_R1]
 cmp ax,[bp+FR_E1]
 jmp o_okfail

show_w2:
 mov ax,[bp+FR_R1]
 call o_hex4
 call o_sp
 mov ax,[bp+FR_R2]
 call o_hex4
 P "  erw. "
 mov ax,[bp+FR_E1]
 call o_hex4
 call o_sp
 mov ax,[bp+FR_E2]
 call o_hex4
 mov ax,[bp+FR_R1]
 cmp ax,[bp+FR_E1]
 jne .n
 mov ax,[bp+FR_R2]
 cmp ax,[bp+FR_E2]
.n:
 jmp o_okfail

; ---------------------------------------------------------------- Rahmen
; Wird von view_exposed angesprungen: CX = Fenster.
k12_expose:
 push bp
 sub sp,FRAME
 mov bp,sp
 push ss
 pop ds
 mov di,cx
 KCALL 211                 ; GrCreateState
 mov [bp+FR_WIN],di
 KCALL 446                 ; GrBeginUpdate
 mov ax,15
 KCALL 355                 ; GrSetAreaColor weiss
 xor ax,ax
 xor bx,bx
 mov cx,640
 mov dx,400
 KCALL 336                 ; GrFillRect
 xor ax,ax
 KCALL 359                 ; GrSetTextColor schwarz
 mov word [bp+FR_Y],4
 mov word [bp+FR_LEN],0
 mov di,[bp+FR_WIN]
 KCALL 293                 ; GrGetFont -> CX=FontID
 mov [bp+FR_FONT],cx
 call run_tests
 mov di,[bp+FR_WIN]
 KCALL 447                 ; GrEndUpdate
 KCALL 212                 ; GrDestroyState
 add sp,FRAME
 pop bp
 retf

; ---------------------------------------------------------------- LMem
; Neuer LMem-Block wie in PS.GEO. Aus: BX=Handle, AX=Chunk (Groesse 0), DS=SS
new_lmem_block:
 mov ax,0x64
 mov cx,0x6050
 KCALL 0                   ; MemAlloc (gesperrt) -> BX, AX=Segment
 push bx
 mov ds,ax
 xor ax,ax
 mov cx,0x060a
 mov dx,0x10
 KCALL 24                  ; LMemInitHeap
 xor cx,cx
 KCALL 25                  ; LMemAlloc -> AX=Chunk
 pop bx
 push ax
 KCALL 6                   ; MemUnlock
 pop ax
 push ss
 pop ds
 ret

; ---------------------------------------------------------------- GString aufzeichnen
; Legt Block A an und zeichnet acht Elemente auf. Ergebnis in FR_BLK/FR_CHK.
rec_gstring:
 call new_lmem_block
 mov [bp+FR_BLK],bx
 mov [bp+FR_CHK],ax
 mov si,ax
 mov cl,0                  ; GST_CHUNK
 KCALL 281                 ; GrCreateGString -> DI
 mov [bp+FR_REC],di
 call o_reset
 P "281 GrCreateGString -> DI="
 mov ax,[bp+FR_REC]
 call o_hex4
 P "  Block="
 mov ax,[bp+FR_BLK]
 call o_hex4
 P " Chunk="
 mov ax,[bp+FR_CHK]
 call o_hex4
 call o_line
 mov di,[bp+FR_REC]
 mov ax,1
 KCALL 346                 ; GrSetLineColor Index 1
 mov di,[bp+FR_REC]
 mov ax,360
 mov bx,300
 mov cx,600
 mov dx,300
 KCALL 314                 ; GrDrawLine
 mov di,[bp+FR_REC]
 mov ax,4
 KCALL 355                 ; GrSetAreaColor Index 4
 mov di,[bp+FR_REC]
 mov ax,360
 mov bx,310
 mov cx,400
 mov dx,340
 KCALL 336                 ; GrFillRect
 mov di,[bp+FR_REC]
 mov cx,[bp+FR_FONT]
 mov dx,14
 xor ax,ax
 KCALL 366                 ; GrSetFont 14 pt
 mov di,[bp+FR_REC]
 push ds
 push cs
 pop ds
 mov si,str_ab
 mov cx,2
 mov ax,410
 mov bx,315
 KCALL 330                 ; GrDrawText "Ab"
 pop ds
 mov di,[bp+FR_REC]
 mov ax,440
 mov bx,315
 mov dx,'Z'
 KCALL 328                 ; GrDrawChar 'Z'
 mov di,[bp+FR_REC]
 mov ax,470
 mov bx,305
 mov cx,560
 mov dx,345
 KCALL 333                 ; GrDrawEllipse
 mov di,[bp+FR_REC]
 mov bx,[bp+FR_BLK]
 mov si,[bp+FR_CHK]
 KCALL 301                 ; GrEndString
 mov di,[bp+FR_REC]
 mov bx,[bp+FR_BLK]
 mov si,[bp+FR_CHK]
 mov dl,1
 KCALL 279                 ; GrDestroyGString, Daten behalten
 push ss
 pop ds
 call o_reset
 P "301 GrEndString, 279 GrDestroyGString(DL=1) ausgefuehrt"
 call o_line
 ret

str_ab: db 'Ab'

; ---------------------------------------------------------------- Elemente
; Laenge der Argumente je Opcode (ohne Opcode-Byte), laut GSTRING12.md.
; FFh = variabel (Text), FEh = Ende.
op_table:
 db 0x00,0xfe, 0x4e,1, 0x32,3, 0x50,1, 0x3b,3, 0x0d,8, 0x28,8
 db 0x46,5, 0x1f,0xff, 0x20,0xff, 0x1d,5, 0x24,8, 0x2f,1, 0x52,1, 0x3f,3
 db 0x31,4, 0x0e,4, 0x10,4, 0x29,4, 0x2e,8, 0x0f,8, 0x11,6, 0x13,6
 db 0x12,2, 0x14,2, 0x1e,1, 0x17,4, 0x18,0, 0x35,2, 0x04,0, 0x05,0
op_table_end:

; Laenge des Elements an ES:SI (inkl. Opcode) -> CX, CF=1 wenn unbekannt
elem_len:
 push bx
 mov al,[es:si]
 mov bx,op_table
.l:
 cmp bx,op_table_end
 jae .unk
 cmp al,[cs:bx]
 je .hit
 add bx,2
 jmp .l
.hit:
 mov cl,[cs:bx+1]
 xor ch,ch
 cmp cl,0xfe
 jne .nend
 mov cx,1
 jmp .ok
.nend:
 cmp cl,0xff
 jne .fix
 cmp al,0x1f               ; DRAW_TEXT: x,y,n
 jne .cp
 mov cx,[es:si+5]
 add cx,7
 jmp .ok
.cp:
 mov cx,[es:si+1]          ; DRAW_TEXT_CP: n
 add cx,3
 jmp .ok
.fix:
 inc cx
.ok:
 clc
 pop bx
 ret
.unk:
 stc
 pop bx
 ret

; CX Bytes ab ES:SI hex ausgeben (hoechstens 24)
o_bytes:
 push si
 cmp cx,24
 jbe .l
 mov cx,24
.l:
 jcxz .d
 mov al,[es:si]
 call o_hex2
 call o_sp
 inc si
 dec cx
 jmp .l
.d:
 pop si
 ret

; Chunk FR_CHK in Block FR_BLK zerlegen und anzeigen.
; Achtung: P veraendert SI, daher Cursor in FR_R1, Laenge in FR_R2.
dump_gstring:
 mov bx,[bp+FR_BLK]
 KCALL 5                   ; MemLock
 mov [bp+FR_SEG],ax
 mov es,ax
 mov si,[bp+FR_CHK]
 mov si,[es:si]
 mov [bp+FR_R1],si
 mov cx,[es:si-2]
 mov [bp+FR_R2],cx
 add cx,si
 mov [bp+FR_END],cx
 call o_reset
 P "Chunk: Offset="
 mov ax,[bp+FR_R1]
 call o_hex4
 P " Laengenwort="
 mov ax,[bp+FR_R2]
 call o_hex4
 call o_line
 mov word [bp+FR_N],0
.el:
 mov es,[bp+FR_SEG]
 mov si,[bp+FR_R1]
 cmp si,[bp+FR_END]
 jae .done
 cmp word [bp+FR_N],14
 jae .done
 inc word [bp+FR_N]
 call elem_len
 jc .unknown
 mov [bp+FR_R2],cx
 call o_reset
 P " op "
 mov es,[bp+FR_SEG]
 mov si,[bp+FR_R1]
 mov al,[es:si]
 call o_hex2
 P " len "
 mov ax,[bp+FR_R2]
 call o_hex2
 P " : "
 mov es,[bp+FR_SEG]
 mov si,[bp+FR_R1]
 mov cx,[bp+FR_R2]
 call o_bytes
 call o_line
 mov es,[bp+FR_SEG]
 mov si,[bp+FR_R1]
 mov al,[es:si]
 add si,[bp+FR_R2]
 mov [bp+FR_R1],si
 or al,al
 jnz .el
 jmp .done
.unknown:
 call o_reset
 P " ?? unbekannter Opcode, Rest roh: "
 mov es,[bp+FR_SEG]
 mov si,[bp+FR_R1]
 mov cx,[bp+FR_END]
 sub cx,si
 call o_bytes
 call o_line
.done:
 mov bx,[bp+FR_BLK]
 KCALL 6                   ; MemUnlock
 push ss
 pop es
 ret
