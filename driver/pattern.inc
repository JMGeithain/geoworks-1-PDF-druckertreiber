; ============================================================ Kachelmuster
; Flaechen mit den GEOS-Mustern 1..24 (Gitter, Linien, Ziegel ...) werden als
; ungefaerbte PDF-Kachelmuster (PaintType 2) gefuellt: 8 x 8 Punkt je Kachel,
; 1 Musterbit = 1 Punkt wie am Bildschirm, Farbe = Flaechenfarbe.

; CX = Musternummer -> ZF = 0 wenn benutzt
pat_used:
 push ax
 push cx
 mov ax,1
 cmp cl,16
 jae .hi
 shl ax,cl
 test [V_PUSED],ax
 jmp .x
.hi:
 sub cl,16
 shl ax,cl
 test [V_PUSED+2],ax
.x:
 pop cx
 pop ax
 ret

; AX = naechste freie Objektnummer -> Objektnummern vergeben (2 je Muster)
pats_alloc:
 push cx
 mov [V_POBJ],ax
 mov cx,1
.l:
 call pat_used
 jz .n
 add ax,2
.n:
 inc cx
 cmp cx,25
 jb .l
 pop cx
 ret

; "/Pattern << /Pn obj 0 R ... >>" fuer das Ressourcenobjekt
pats_res:
 push ax
 push cx
 push dx
 mov ax,[V_PUSED]
 or ax,[V_PUSED+2]
 jz .x
 OUT "/Pattern <<"
 mov dx,[V_POBJ]
 mov cx,1
.l:
 call pat_used
 jz .n
 OUT " /P"
 mov ax,cx
 call out_int
 OUT " "
 mov ax,dx
 call out_int
 OUT " 0 R"
 add dx,2
.n:
 inc cx
 cmp cx,25
 jb .l
 OUT " >>",10
.x:
 pop dx
 pop cx
 pop ax
 ret

; Musterobjekte schreiben
pats_write:
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 mov dx,[V_POBJ]
 mov cx,1
.l:
 call pat_used
 jz .n
 mov ax,dx
 call obj_start
 OUT "<< /PatternType 1 /PaintType 2 /TilingType 1 /BBox [0 0 8 8]"
 OUT " /XStep 8 /YStep 8 /Resources << >> /Length "
 mov ax,dx
 inc ax
 call out_int
 OUT " 0 R >>",10,"stream",10
 mov ax,[V_POS]
 mov [V_SSTART],ax
 mov ax,[V_POS+2]
 mov [V_SSTART+2],ax
 ; Kachel: Zeile r (0 = oben) liegt bei y = 7 - r
 mov si,cx
 shl si,1
 shl si,1
 shl si,1
 add si,pat_bits
 xor bx,bx                      ; BX = Zeile
.row:
 mov ah,[cs:si+bx]
 xor di,di                      ; DI = Spalte
.col:
 test ah,0x80
 jz .nb
 push ax
 mov ax,di
 call out_int
 OUT " "
 mov ax,7
 sub ax,bx
 call out_int
 OUT " 1 1 re "
 pop ax
.nb:
 shl ah,1
 inc di
 cmp di,8
 jb .col
 inc bx
 cmp bx,8
 jb .row
 OUT "f"
 mov ax,[V_POS]
 mov bx,[V_POS+2]
 sub ax,[V_SSTART]
 sbb bx,[V_SSTART+2]
 push bx
 push ax
 OUT 10,"endstream",10,"endobj",10
 mov ax,dx
 inc ax
 call obj_start
 pop ax
 pop bx
 push dx
 mov dx,bx
 push si
 xor si,si
 call out_u32
 pop si
 pop dx
 OUT 10,"endobj",10
 add dx,2
.n:
 inc cx
 cmp cx,25
 jb .l
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 ret

%include "patterns.inc"
