; SPDX-License-Identifier: MIT
; Meilenstein 1: Elemente -> PDF mit Objekt-Transformation (Festkomma 16.16).
; ES:SI = kopiertes Element, V_ELEN = Laenge. Die aktuelle Matrix des
; Hilfs-GState steht in V_OM (6 x WWFixed: e11 e12 e21 e22 e31 e32).
; Punkte werden hier selbst transformiert; der PDF-CTM enthaelt nur die
; Seitenspiegelung und die Seitentransformation aus ESC 8100h.

%macro NEG32 2                 ; %1:%2 = hi:lo
 neg %2
 adc %1,0
 neg %1
%endmacro

; ------------------------------------------------------------ Festkomma
; DX:CX = DX:CX * [BX] (beide 16.16, vorzeichenbehaftet)
mulfix:
 push ax
 push bx
 push si
 push di
 push bp
 mov si,[bx]
 mov di,[bx+2]
 xor bp,bp
 test dx,0x8000
 jz .a
 NEG32 dx,cx
 xor bp,1
.a:
 test di,0x8000
 jz .b
 NEG32 di,si
 xor bp,1
.b:
 push dx                       ; Ah
 push cx                       ; Al
 mov ax,cx
 mul si                        ; Al*Bl
 mov bx,dx
 xor cx,cx
 pop ax                        ; Al
 mul di                        ; Al*Bh
 add bx,ax
 adc cx,dx
 pop ax                        ; Ah
 push ax
 mul si                        ; Ah*Bl
 add bx,ax
 adc cx,dx
 pop ax
 mul di                        ; Ah*Bh
 add cx,ax
 mov dx,cx
 mov cx,bx
 test bp,1
 jz .c
 NEG32 dx,cx
.c:
 pop bp
 pop di
 pop si
 pop bx
 pop ax
 ret

; [V_PX],[V_PY] (16.16) mit V_OM transformieren -> [V_QX],[V_QY]
xform:
 push ax
 push bx
 push cx
 push dx
 mov dx,[V_PX+2]
 mov cx,[V_PX]
 mov bx,V_OM
 call mulfix
 mov [V_QX],cx
 mov [V_QX+2],dx
 mov dx,[V_PY+2]
 mov cx,[V_PY]
 mov bx,V_OM+8
 call mulfix
 add [V_QX],cx
 adc [V_QX+2],dx
 mov ax,[V_OM+16]
 add [V_QX],ax
 mov ax,[V_OM+18]
 adc [V_QX+2],ax
 mov dx,[V_PX+2]
 mov cx,[V_PX]
 mov bx,V_OM+4
 call mulfix
 mov [V_QY],cx
 mov [V_QY+2],dx
 mov dx,[V_PY+2]
 mov cx,[V_PY]
 mov bx,V_OM+12
 call mulfix
 add [V_QY],cx
 adc [V_QY+2],dx
 mov ax,[V_OM+20]
 add [V_QY],ax
 mov ax,[V_OM+22]
 adc [V_QY+2],ax
 pop dx
 pop cx
 pop bx
 pop ax
 ret

; Transformierten Punkt V_PX/V_PY ausgeben: "x y "
out_qpt:
 push cx
 push dx
 call xform
 mov cx,[V_QX]
 mov dx,[V_QX+2]
 call out_fix
 OUT " "
 mov cx,[V_QY]
 mov dx,[V_QY+2]
 call out_fix
 OUT " "
 pop dx
 pop cx
 ret

; Ganzzahlpunkt AX=x, BX=y transformiert ausgeben
out_ipt:
 mov word [V_PX],0
 mov [V_PX+2],ax
 mov word [V_PY],0
 mov [V_PY+2],bx
 jmp out_qpt

; ------------------------------------------------------------ Verteiler
emit_elem:
 mov al,[es:si]
 mov ah,[V_INV]                 ; Maske SDM_0: Element nicht zeichnen
 test ah,2
 jz .ia
 cmp al,0x28
 je .skip
 cmp al,0x2d
 je .skip
 cmp al,0x2e
 je .skip
.ia:
 test ah,1
 jz .il
 cmp al,0x0d
 je .skip
 cmp al,0x11
 je .skip
 cmp al,0x13
 je .skip
 cmp al,0x0f
 je .skip
 cmp al,0x23
 je .skip
 cmp al,0x27
 je .skip
 cmp al,0x24
 je .skip
.il:
 test ah,4
 jz .go
 cmp al,0x1f
 je .skip
 cmp al,0x20
 je .skip
 cmp al,0x1d
 je .skip
 jmp .go
.skip:
 ret
.go:
 cmp al,0x0d
 je e_line
 cmp al,0x11
 je e_hline
 cmp al,0x13
 je e_vline
 cmp al,0x0f
 je e_rect
 cmp al,0x28
 je e_fillrect
 cmp al,0x23
 je e_polyline
 cmp al,0x27
 je e_polygon
 cmp al,0x2d
 je e_fillpoly
 cmp al,0x24
 je e_ellipse
 cmp al,0x2e
 je e_fillellipse
 cmp al,0x1f
 je e_text
 cmp al,0x20
 je e_textcp
 cmp al,0x1d
 je e_char
 cmp al,0x21
 je e_textfield
 cmp al,0x19
 je e_bitmap
 cmp al,0x1a
 je e_bitmapcp
 jmp dump_elem

; "% op XX len NN: bytes" (hoechstens 48 Bytes)
dump_elem:
 push cx
 push di
 OUT "% op "
 mov al,[es:si]
 call out_hex2
 OUT " len "
 mov ax,[V_ELEN]
 call out_int
 OUT ":"
 mov cx,[V_ELEN]                ; nur die ersten 16 Bytes (unbekannter Befehl)
 cmp cx,16
 jbe .n
 mov cx,16
.n:
 mov di,si
.l:
 jcxz .e
 OUT " "
 mov al,[es:di]
 call out_hex2
 inc di
 dec cx
 jmp .l
.e:
 OUT 10
 pop di
 pop cx
 ret

; Ganzes Element als Kommentar (32 Byte je Zeile), zur Analyse
dump_full:
 push ax
 push cx
 push dx
 push di
 OUT "% op "
 mov al,[es:si]
 call out_hex2
 OUT " len "
 mov ax,[V_ELEN]
 xor dx,dx
 push si
 xor si,si
 call out_u32
 pop si
 mov cx,[V_ELEN]
 mov di,si
 xor dx,dx
.l:
 jcxz .e
 test dl,31
 jnz .s
 OUT 10,"%X"
.s:
 OUT " "
 mov al,[es:di]
 call out_hex2
 inc di
 inc dx
 dec cx
 jmp .l
.e:
 OUT 10
 pop di
 pop dx
 pop cx
 pop ax
 ret

stroke_setup:
 mov bx,V_LC
 call out_rgb
 OUT " RG "
 mov ax,[V_LW]
 call out_int
 OUT " w",10
 ret
fill_setup:
 mov bx,V_AC
 cmp byte [V_APAT],0
 jne .pat
 call out_rgb
 OUT " rg",10
 ret
.pat:                           ; Kachelmuster in der Flaechenfarbe
 OUT "/CsP cs "
 call out_rgb
 OUT " /P"
 push ax
 push cx
 mov al,[V_APAT]
 xor ah,ah
 call out_int
 mov cl,al
 mov ax,1
 cmp cl,16
 jae .hi
 shl ax,cl
 or [V_PUSED],ax
 jmp .m
.hi:
 sub cl,16
 shl ax,cl
 or [V_PUSED+2],ax
.m:
 pop cx
 pop ax
 OUT " scn",10
 ret

e_line:
 call stroke_setup
 mov ax,[es:si+1]
 mov bx,[es:si+3]
 call out_ipt
 OUT "m "
 mov ax,[es:si+5]
 mov bx,[es:si+7]
 call out_ipt
 OUT "l S",10
 ret
e_hline:                        ; x1 y x2
 call stroke_setup
 mov ax,[es:si+1]
 mov bx,[es:si+3]
 call out_ipt
 OUT "m "
 mov ax,[es:si+5]
 mov bx,[es:si+3]
 call out_ipt
 OUT "l S",10
 ret
e_vline:                        ; x y1 y2
 call stroke_setup
 mov ax,[es:si+1]
 mov bx,[es:si+3]
 call out_ipt
 OUT "m "
 mov ax,[es:si+1]
 mov bx,[es:si+5]
 call out_ipt
 OUT "l S",10
 ret
rect_path:                      ; l t r b -> vier transformierte Ecken
 mov ax,[es:si+1]
 mov bx,[es:si+3]
 call out_ipt
 OUT "m "
 mov ax,[es:si+5]
 mov bx,[es:si+3]
 call out_ipt
 OUT "l "
 mov ax,[es:si+5]
 mov bx,[es:si+7]
 call out_ipt
 OUT "l "
 mov ax,[es:si+1]
 mov bx,[es:si+7]
 call out_ipt
 OUT "l h "
 ret
e_rect:
 call stroke_setup
 call rect_path
 OUT "S",10
 ret
e_fillrect:
 call fill_setup
 call rect_path
 OUT "f",10
 ret
; Punktliste: CX Anzahl, Punkte ab ES:DI
poly_path:
 jcxz .x
 mov ax,[es:di]
 mov bx,[es:di+2]
 call out_ipt
 OUT "m "
 add di,4
 dec cx
.l:
 jcxz .x
 mov ax,[es:di]
 mov bx,[es:di+2]
 call out_ipt
 OUT "l "
 add di,4
 dec cx
 jmp .l
.x:
 ret
e_polyline:
 call stroke_setup
 mov cx,[es:si+1]
 lea di,[si+3]
 call poly_path
 OUT "S",10
 ret
e_polygon:
 call stroke_setup
 mov cx,[es:si+1]
 lea di,[si+3]
 call poly_path
 OUT "h S",10
 ret
e_fillpoly:                     ; Regel, Anzahl, Punkte
 call fill_setup
 mov cx,[es:si+2]
 lea di,[si+4]
 call poly_path
 cmp byte [es:si+1],0
 jne .w
 OUT "f*",10
 ret
.w:
 OUT "f",10
 ret

; ------------------------------------------------------------ Ellipse
e_ellipse:
 call stroke_setup
 mov word [V_FILL],0
 jmp ellipse
e_fillellipse:
 call fill_setup
 mov word [V_FILL],1
ellipse:                        ; l t r b -> Mittelpunkt/Radien in 16.16
 mov ax,[es:si+1]
 mov bx,[es:si+5]
 call half_sum                  ; DX:CX = (l+r)/2
 mov [V_ECX],cx
 mov [V_ECX+2],dx
 mov ax,[es:si+3]
 mov bx,[es:si+7]
 call half_sum
 mov [V_ECY],cx
 mov [V_ECY+2],dx
 mov ax,[es:si+5]
 mov bx,[es:si+1]
 call half_diff                 ; (r-l)/2
 mov [V_ERX],cx
 mov [V_ERX+2],dx
 mov bx,V_KAP
 call mulfix
 mov [V_EKX],cx
 mov [V_EKX+2],dx
 mov ax,[es:si+7]
 mov bx,[es:si+3]
 call half_diff
 mov [V_ERY],cx
 mov [V_ERY+2],dx
 mov bx,V_KAP
 call mulfix
 mov [V_EKY],cx
 mov [V_EKY+2],dx
 xor cx,cx
.p:
 push cx
 mov bx,cx
 mov al,[cs:ell_x+bx]
 mov di,V_ECX
 call ell_coord                 ; -> DX:CX
 mov [V_PX],cx
 mov [V_PX+2],dx
 pop cx
 push cx
 mov bx,cx
 mov al,[cs:ell_y+bx]
 mov di,V_ECY
 call ell_coord
 mov [V_PY],cx
 mov [V_PY+2],dx
 call out_qpt
 pop cx
 cmp cx,0
 jne .c1
 OUT "m "
 jmp .nx
.c1:
 mov ax,cx
 mov bl,3
 div bl
 or ah,ah
 jnz .nx
 OUT "c",10
.nx:
 inc cx
 cmp cx,13
 jb .p
 cmp word [V_FILL],0
 jne .f
 OUT "S",10
 ret
.f:
 OUT "f",10
 ret
half_sum:                       ; DX:CX = (AX+BX)/2 als 16.16
 cwd
 mov cx,dx
 push ax
 mov ax,bx
 cwd
 pop bx
 add ax,bx
 adc dx,cx                      ; DX:AX = Summe (32 Bit)
 mov cx,0
 sar dx,1
 rcr ax,1
 rcr cx,1
 mov dx,ax
 ret
half_diff:                      ; DX:CX = (AX-BX)/2
 neg bx
 jmp half_sum
; AL Code, DI = Basis (16.16) -> DX:CX = Basis +/- Versatz
ell_coord:
 push ax
 push bx
 mov cx,[di]
 mov dx,[di+2]
 mov ah,al
 and al,0x7f
 jz .x
 xor bx,bx
 cmp al,1
 jne .a
 mov bx,V_ERX
.a:
 cmp al,2
 jne .b
 mov bx,V_ERY
.b:
 cmp al,3
 jne .c
 mov bx,V_EKX
.c:
 cmp al,4
 jne .d
 mov bx,V_EKY
.d:
 test ah,0x80
 jnz .m
 add cx,[bx]
 adc dx,[bx+2]
 jmp .x
.m:
 sub cx,[bx]
 sbb dx,[bx+2]
.x:
 pop bx
 pop ax
 ret
ell_x: db 1,1,3,0,0x83,0x81,0x81,0x81,0x83,0,3,1,1
ell_y: db 0,4,2,2,2,4,0,0x84,0x82,0x82,0x82,0x84,0

; ------------------------------------------------------------ Text
; Textanfang: AX=x, BX=y (Oberkante), DX = Groesse (ganzzahlig).
; Schreibt "BT /F1 g Tf a b c d X Y Tm " mit Grundlinie ~ y + 3/4 g.
text_begin:
 push cx
 push dx
 or dx,dx
 jnz .s
 mov dx,12
.s:
 mov [V_TS],dx
 push dx
 shr dx,1
 add bx,dx
 shr dx,1
 add bx,dx
 pop dx
 mov word [V_PX],0
 mov [V_PX+2],ax
 mov word [V_PY],0
 mov [V_PY+2],bx
 call tm_begin
 pop dx
 pop cx
 ret
; "BT /Fn g Tf a b c d X Y Tm " mit V_FNUM, V_TS und Punkt V_PX/V_PY
tm_begin:
 push ax
 push cx
 push dx
 OUT "BT /F"
 mov ax,[V_FNUM]
 call out_int
 OUT " "
 mov ax,[V_TS]
 call out_int
 OUT " Tf "
 mov cx,[V_OM]                  ; e11
 mov dx,[V_OM+2]
 call out_fix
 OUT " "
 mov cx,[V_OM+4]                ; e12
 mov dx,[V_OM+6]
 call out_fix
 OUT " "
 mov cx,[V_OM+8]                ; -e21
 mov dx,[V_OM+10]
 NEG32 dx,cx
 push bx
 mov bx,V_OM                    ; + Schraegung * e11
 call skew_add
 pop bx
 call out_fix
 OUT " "
 mov cx,[V_OM+12]               ; -e22
 mov dx,[V_OM+14]
 NEG32 dx,cx
 push bx
 mov bx,V_OM+4                  ; + Schraegung * e12
 call skew_add
 pop bx
 call out_fix
 OUT " "
 call out_qpt
 OUT "Tm "
 pop dx
 pop cx
 pop ax
 ret
; CX Zeichen ab ES:DI als "(..) Tj "
text_chars:
 OUT "("
.l:
 jcxz .e
 mov al,[es:di]
 cmp al,32                      ; Tabulator, CR usw. nicht ausgeben
 jb .sk
 call mark_used
 call out_pdfchar
.sk:
 inc di
 dec cx
 jmp .l
.e:
 OUT ") Tj "
 ret
text_color_g:                   ; Textfarbe aus dem GState
 mov bx,V_TC
 call out_rgb
 OUT " rg "
 call out_rgb
 OUT " RG "
 ret

e_text:                         ; x y n Zeichen
 call font_from_g
 call text_color_g
 mov ax,[es:si+1]
 mov bx,[es:si+3]
 mov dx,[V_FSIZE]
 call text_begin
 mov cx,[es:si+5]
 lea di,[si+7]
 call text_chars
 OUT "ET",10
 ret
e_textcp:                       ; n Zeichen an aktueller Position
 call font_from_g
 call text_color_g
 mov ax,[V_CPX]
 mov bx,[V_CPY]
 mov dx,[V_FSIZE]
 call text_begin
 mov cx,[es:si+1]
 lea di,[si+3]
 call text_chars
 OUT "ET",10
 ret
e_char:                         ; Zeichen x y
 call font_from_g
 call text_color_g
 mov ax,[es:si+2]
 mov bx,[es:si+4]
 mov dx,[V_FSIZE]
 call text_begin
 mov cx,1
 lea di,[si+1]
 call text_chars
 OUT "ET",10
 ret

; Text-Feld (Opcode 21h): fcount, OldGDF_saved (28 Byte), Zeichen, Stil-Abschnitte.
; Abschnitt = Anzahl (Wort) + OldTextAttr (20 oder 19 Byte, siehe GSTRING12.md).
TF_SAVED equ 3                  ; Beginn von OldGDF_saved im Element
e_textfield:
 mov ax,[es:si+1]               ; fcount
 sub ax,28
 mov [V_TFN],ax                 ; = Zeichen + erster Abschnitt
 mov word [V_TFR],22
 call tf_try
 jnc .ok
 mov word [V_TFR],21
 call tf_try
 jnc .ok
 ; nur Grafik (Zeichen 1Ah ohne Textabschnitt): eingebetteten GString zeichnen
 mov ax,[es:si+TF_SAVED+5]
 add ax,[es:si+TF_SAVED+22]
 add ax,[es:si+TF_SAVED+19]
 mov [V_TFX],ax
 mov bx,[es:si+1]               ; hinter dem festen Teil (ein Abschnitt)
 add bx,3
 call tf_embedded
 jnc .gr
 OUT "% Textfeld nicht erkannt",10
.gr:
 ret
.ok:
 ; Startpunkt: x = position + textLeft + adjustment
 mov ax,[es:si+TF_SAVED+5]
 add ax,[es:si+TF_SAVED+22]
 add ax,[es:si+TF_SAVED+19]
 mov [V_TFX],ax
 mov bx,1000
 imul bx
 mov [V_XB],ax
 mov [V_XB+2],dx
 call tf_reset
 ; Grundlinie: yPos + baseLine, sonst yPos + 3/4 Groesse des ersten Abschnitts
 mov di,[V_TFP]
 call tf_size
 mov bx,[es:si+TF_SAVED+24]
 test byte [es:di+2+8],0x08     ; TM_DRAW_BASE: yPos ist die Grundlinie
 jnz .bs
 mov ax,[es:si+TF_SAVED+26]
 or ax,ax
 jz .nb
 add bx,ax
 jmp .bs
.nb:
 shr dx,1
 add bx,dx
 shr dx,1
 add bx,dx
.bs:
 mov [V_TBASE],bx
 ; ---- Durchgang 1: PDF-Breite der Zeile messen
 xor ax,ax
 mov [V_ACC],ax
 mov [V_ACC+2],ax
 mov [V_NCH],ax
.m_run:
 call tf_next                   ; CX = Zeichen, BX = Zeiger, CF = Ende
 jc .m_end
.m_c:
 jcxz .m_run
 mov al,[es:bx]
 cmp al,32
 jb .m_sk
 call char_w
 mul word [V_RSZ]
 add [V_ACC],ax
 adc [V_ACC+2],dx
 inc word [V_NCH]
.m_sk:
 inc bx
 dec cx
 jmp .m_c
.m_end:
 ; Zeichenabstand Tc = (GEOS-Breite*1000 - PDF-Breite) / Zeichenzahl (Milli-Punkt)
 mov word [V_TC2],0
 mov ax,[es:si+TF_SAVED+7]
 or ax,ax
 jle .notc
 mov cx,[V_NCH]
 jcxz .notc
 mov bx,1000
 mul bx
 sub ax,[V_ACC]
 sbb dx,[V_ACC+2]
 xor bx,bx
 or dx,dx
 jns .tp
 NEG32 dx,ax
 inc bx
.tp:
 cmp dx,cx
 jae .notc
 div cx
 cmp ax,15000
 ja .notc
 or bx,bx
 jz .ts
 neg ax
.ts:
 mov [V_TC2],ax
.notc:
 ; ---- Durchgang 2: je Abschnitt ein Textobjekt, danach Linien
 xor ax,ax
 mov [V_XM],ax
 mov [V_XM+2],ax
 call tf_reset
 OUT "q "
 mov ax,[V_TC2]
 or ax,ax
 jz .o_run
 cwd
 call milli2fix
 call out_fix
 OUT " Tc "
.o_run:
 call tf_next
 jc .o_end
 push bx
 push cx
 mov ax,[V_XM]
 mov dx,[V_XM+2]
 mov [V_X0],ax
 mov [V_X0+2],dx
 mov ax,[V_TBASE]
 mov bx,1000
 imul bx
 add ax,[V_RSH]
 adc dx,[V_RSH+2]
 mov [V_YA],ax
 mov [V_YA+2],dx
 mov ax,[V_XM]
 mov dx,[V_XM+2]
 call set_mpt
 mov ax,[V_RSZ]
 mov [V_TS],ax
 call tm_begin
 mov di,[V_RUN]
 call tf_color
 pop cx
 pop di
 push di
 push cx
 call text_chars
 OUT "ET",10
 pop cx
 pop bx
.o_c:
 jcxz .o_dec
 mov al,[es:bx]
 cmp al,32
 jb .o_sk
 call char_w
 mul word [V_RSZ]
 add [V_XM],ax
 adc [V_XM+2],dx
 mov ax,[V_TC2]
 cwd
 add [V_XM],ax
 adc [V_XM+2],dx
.o_sk:
 inc bx
 dec cx
 jmp .o_c
.o_dec:
 test byte [V_RSTY],3
 jz .o_run
 call deco_lines
 jmp .o_run
.o_end:
 OUT "Q",10
 mov bx,[V_TFP]                 ; hinter dem letzten Abschnitt
 sub bx,si
 call tf_embedded               ; eingebettete Grafik am Zeilenanfang
 ret

; Abschnitts-Iterator zuruecksetzen
tf_reset:
 mov ax,[V_TFN]
 add ax,si
 add ax,31
 mov [V_TFP],ax
 lea ax,[si+31]
 mov [V_TFC],ax
 mov ax,[V_TFN]
 mov [V_TFREM],ax
 ret
; Naechster Abschnitt: CX = Zeichenanzahl, BX = Zeichenzeiger, setzt V_FNUM,
; V_RSZ, V_RSTY, V_RUN. CF = 1 am Ende.
tf_next:
 cmp word [V_TFREM],0
 je .end
 mov di,[V_TFP]
 mov ax,di
 add ax,[V_TFR]
 sub ax,si
 cmp ax,[V_ELEN]
 ja .end
 mov [V_RUN],di
 mov cx,[es:di]
 cmp cx,[V_TFREM]
 jbe .c
 mov cx,[V_TFREM]
.c:
 sub [V_TFREM],cx
 call tf_font
 call tf_size
 mov [V_RSZ],dx
 mov al,[es:di+2+6]
 mov [V_RSTY],al
 mov word [V_RSH],0             ; Hoch-/Tiefstellung: halbe Groesse,
 mov word [V_RSH+2],0           ; Versatz -0,375 bzw. +0,1 der Groesse
 test al,0x0c
 jz .ns
 push cx
 mov cx,100
 test al,8
 jz .sb
 mov cx,-375
.sb:
 mov ax,[V_RSZ]
 imul cx
 mov [V_RSH],ax
 mov [V_RSH+2],dx
 shr word [V_RSZ],1
 jnz .nz
 inc word [V_RSZ]
.nz:
 pop cx
.ns:
 mov ax,[V_TFR]
 add [V_TFP],ax
 mov bx,[V_TFC]
 add [V_TFC],cx
 clc
 ret
.end:
 stc
 ret

; AL = Zeichen -> AX = Breite in 1/1000 em fuer Schrift V_FNUM
char_w:
 push bx
 cmp al,32
 jb .zero
 mov bx,[V_FNUM]
 cmp bx,15
 jb .std
 sub bx,15                      ; eingebettete Schrift: Breite aus dem Platz
 push cx
 mov cl,9
 shl bx,cl
 pop cx
 sub al,32
 xor ah,ah
 add bx,ax
 add bx,ax
 mov ax,[bx+SLOTS+S_W]
 pop bx
 ret
.std:
 cmp bx,13
 jb .nsym
 sub bx,7                       ; 13 -> 6, 14 -> 7
 jmp .t
.nsym:
 cmp bx,9
 jae .cour
 dec bx
 cmp bx,4
 jb .t
 and bx,1
 add bx,4
.t:
 shl bx,1
 mov bx,[cs:wt_off+bx]
 sub al,32
 xor ah,ah
 add bx,ax
 mov al,[cs:wtab+bx]
 xor ah,ah
 shl ax,1
 shl ax,1
 pop bx
 ret
.cour:
 mov ax,600
 pop bx
 ret
.zero:
 xor ax,ax
 pop bx
 ret

; DX:AX = Milli-Punkte (vorzeichenbehaftet) -> DX:CX = 16.16
milli2fix:
 push bx
 push di
 xor di,di
 or dx,dx
 jns .p
 NEG32 dx,ax
 inc di
.p:
 mov bx,1000
 push ax
 mov ax,dx
 xor dx,dx
 div bx
 pop ax
 div bx
 mov cx,ax
 xor ax,ax
 div bx
 mov dx,cx
 mov cx,ax
 or di,di
 jz .e
 NEG32 dx,cx
.e:
 pop di
 pop bx
 ret
; Punkt setzen: x = V_XB + DX:AX, y = V_YA (alles Milli-Punkte) -> V_PX/V_PY
set_mpt:
 push cx
 push dx
 add ax,[V_XB]
 adc dx,[V_XB+2]
 call milli2fix
 mov [V_PX],cx
 mov [V_PX+2],dx
 mov ax,[V_YA]
 mov dx,[V_YA+2]
 call milli2fix
 mov [V_PY],cx
 mov [V_PY+2],dx
 pop dx
 pop cx
 ret
; Unter-/Durchstreichung des letzten Abschnitts (V_X0 .. V_XM - Tc)
deco_lines:
 test byte [V_RSTY],1
 jz .s
 mov bx,70
 mov cx,120
 call deco_rect
.s:
 test byte [V_RSTY],2
 jz .e
 mov bx,-300
 mov cx,-250
 call deco_rect
.e:
 ret
; BX, CX = obere/untere Kante in Promille der Groesse unter der Grundlinie
deco_rect:
 push cx
 mov ax,[V_RSZ]
 imul bx
 call deco_y
 mov [V_Y0],ax
 mov [V_Y0+2],dx
 pop cx
 mov ax,[V_RSZ]
 imul cx
 call deco_y
 mov [V_Y1],ax
 mov [V_Y1+2],dx
 mov bx,V_Y0
 call deco_x0
 OUT "m "
 call deco_x1
 OUT "l "
 mov bx,V_Y1
 call deco_x1
 OUT "l "
 call deco_x0
 OUT "l h f",10
 ret
deco_y:                         ; DX:AX += Grundlinie*1000
 push bx
 push cx
 mov cx,ax
 mov bx,dx
 mov ax,[V_TBASE]
 mov dx,1000
 imul dx
 add ax,cx
 adc dx,bx
 add ax,[V_RSH]
 adc dx,[V_RSH+2]
 pop cx
 pop bx
 ret
deco_x0:                        ; Punkt (V_X0, [BX]) ausgeben
 mov ax,[bx]
 mov [V_YA],ax
 mov ax,[bx+2]
 mov [V_YA+2],ax
 mov ax,[V_X0]
 mov dx,[V_X0+2]
 call set_mpt
 jmp out_qpt
deco_x1:                        ; Punkt (V_XM - Tc, [BX]) ausgeben
 mov ax,[bx]
 mov [V_YA],ax
 mov ax,[bx+2]
 mov [V_YA+2],ax
 mov ax,[V_TC2]
 cwd
 NEG32 dx,ax
 add ax,[V_XM]
 adc dx,[V_XM+2]
 call set_mpt
 jmp out_qpt

%include "widths.inc"

; Prueft Abschnittsgroesse V_TFR: Zeichenanzahl = V_TFN - R muss >= Anzahl
; im ersten Abschnitt sein. Setzt V_TFN auf die Zeichenanzahl. CF=1 wenn nicht.
tf_try:
 mov ax,[es:si+1]
 sub ax,28
 sub ax,[V_TFR]
 jl .no
 mov di,si
 add di,31
 add di,ax
 mov bx,[es:di]
 or bx,bx
 jz .no
 cmp bx,ax
 ja .no
 mov [V_TFN],ax
 clc
 ret
.no:
 stc
 ret
; ES:DI = Abschnitt -> DX = Schriftgroesse (ganzzahlig)
tf_size:
 push bx
 mov bx,16                      ; 20-Byte-Attr: Groesse ganzzahlig bei +2+16
 cmp word [V_TFR],22
 je .a
 mov bx,15
.a:
 add bx,di
 mov dx,[es:bx+2]
 pop bx
 or dx,dx
 jnz .x
 mov dx,[V_FSIZE]
.x:
 ret
; ES:DI = Abschnitt: Farbe ausgeben (Index ueber GrMapColorIndex)
tf_color:
 push ax
 push bx
 push di
 mov al,[es:di+2]               ; ColorFlag
 or al,al
 jnz .rgb
 mov ah,[es:di+3]               ; Index
 xor al,al
 push es
 push si
 xor di,di
 KCALL 247                      ; GrMapColorIndex -> AL R, BL G, BH B
 DSDG
 pop si
 pop es
 mov [V_RGB],al
 mov [V_RGB+1],bl
 mov [V_RGB+2],bh
 jmp .out
.rgb:
 mov al,[es:di+3]
 mov [V_RGB],al
 mov al,[es:di+4]
 mov [V_RGB+1],al
 mov al,[es:di+5]
 mov [V_RGB+2],al
.out:
 mov bx,V_RGB
 mov al,[es:di+2+4]             ; Textmaske des Abschnitts
 call blend_mask
 call out_rgb
 OUT " rg "
 call out_rgb
 OUT " RG "
 pop di
 pop bx
 pop ax
 ret


; ------------------------------------------------------------ Schriften
; AX = GEOS-FontID, BL = TextStyle -> V_FNUM (1..12)
; Familie = (ID >> 9) & 7: 0 Serif -> Times, 5 Mono -> Courier, sonst Helvetica.
; Stil: 20h fett, 10h kursiv.
font_num:
 ; Umrissschriften ausser URW Roman/Sans/Mono/SymbolPS: einbetten, wenn moeglich
 cmp ax,0x1000
 je .map
 cmp ax,0x1200
 je .map
 cmp ax,0x1a00
 je .map
 cmp ax,0x1800
 je .map
 test ax,0xf000                 ; Bitmap-Schriften
 jz .map
 cmp ax,0xf000                  ; Druckerschriften
 jae .map
 push bx
 and bl,0x30
 call slot_get
 jc .nos
 add bx,15
 mov [V_FNUM],bx
 pop bx
 ret
.nos:
 pop bx
.map:
 push ax
 push cx
 push ax
 mov cl,9
 shr ax,cl
 and ax,7
 pop cx
 cmp al,4                       ; Symbol-Familie: Gesicht 0 Symbol, sonst Dingbats
 jne .ns
 and cx,0x1ff
 mov ax,13
 jcxz .sy
 inc ax
.sy:
 mov [V_FNUM],ax
 pop cx
 pop ax
 ret
.ns:
 mov cx,5                       ; Helvetica
 cmp al,0
 jne .a
 mov cx,1                       ; Times
.a:
 cmp al,5
 jne .b
 mov cx,9                       ; Courier
.b:
 test bl,0x20
 jz .c
 inc cx
.c:
 test bl,0x10
 jz .d
 add cx,2
.d:
 mov [V_FNUM],cx
 pop cx
 pop ax
 ret
font_from_g:
 push ax
 push bx
 mov ax,[V_FID]
 mov bl,[V_STYLE]
 call font_num
 pop bx
 pop ax
 ret
; ES:DI = Stil-Abschnitt -> V_FNUM (FontID bei Attr+13, Stil bei Attr+6)
tf_font:
 push ax
 push bx
 mov ax,[es:di+2+13]
 mov bl,[es:di+2+6]
 call font_num
 pop bx
 pop ax
 ret

; ------------------------------------------------------------ Muster
; AL = SystemDrawMask, [BX] = RGB. SDM_100 (25) .. SDM_0 (89) ergibt
; Deckung (89-m)/64; andere Muster werden als 50 % gerechnet.
; Farbe := 255 - (255 - Farbe) * Deckung / 64
blend_mask:
 push ax
 push cx
 push dx
 push si
 mov cl,64
 or al,al                       ; 0 (SDM_TILE) wie 100 % behandeln
 jz .ok
 mov cl,89
 sub cl,al
 cmp al,25
 jb .pat
 cmp al,89
 jbe .ok
.pat:
 mov cl,32
.ok:
 cmp cl,64
 je .x
 xor ch,ch
 mov si,3
.l:
 mov al,255
 sub al,[bx]
 mul cl                         ; AX = (255-v)*Deckung
 shr ax,1
 shr ax,1
 shr ax,1
 shr ax,1
 shr ax,1
 shr ax,1
 mov ah,255
 sub ah,al
 mov [bx],ah
 inc bx
 dec si
 jnz .l
 sub bx,3
.x:
 cmp cl,1                       ; CF = 1: Deckung 0 (SDM_0, unsichtbar)
 pop si
 pop dx
 pop cx
 pop ax
 ret

; ------------------------------------------------------------ Bitmaps
; 19h: x, y, dann Scheiben (word Groesse, Bitmap); 1Ah: wie 19h ab Stiftposition.
e_bitmap:
 mov ax,[es:si+1]
 mov bx,[es:si+3]
 lea di,[si+5]
 jmp bm_slices
e_bitmapcp:
 mov ax,[V_CPX]
 mov bx,[V_CPY]
 lea di,[si+1]
bm_slices:
 mov [V_BX],ax
 mov [V_BY],bx
 mov ax,si
 add ax,[V_ELEN]
 inc ax
 mov [V_BEND],ax                ; erstes Byte hinter dem Element
.l:
 lea ax,[di+8]
 cmp ax,[V_BEND]
 ja .e
 mov cx,[es:di]                 ; Groesse der Scheibe
 jcxz .e
 add di,2
 push di
 push cx
 call bm_draw
 pop cx
 pop di
 test byte [es:di+5],8          ; einfache Bitmap: nur eine Scheibe
 jz .e
 mov ax,[es:di+6]               ; letzte Scheibe erreicht?
 add ax,[es:di+8]
 cmp ax,[es:di+2]
 jae .e
 add di,cx
 jmp .l
.e:
 ret

; AX = n, BX = Aufloesung -> DX:CX = n*72/Aufl. (16.16)
bm_ratio:
 push ax
 push bx
 mov dx,72
 mul dx
 div bx
 push ax
 xor ax,ax
 div bx
 mov cx,ax
 pop dx
 pop bx
 pop ax
 ret

; ES:DI = Bitmap
bm_draw:
 mov ax,[es:di]
 mov [V_BW],ax
 mov al,[es:di+4]
 mov [V_BCMP],al
 mov al,[es:di+5]
 mov [V_BTYP],al
 and al,7
 mov [V_BFMT],al
 mov word [V_BPAL],0
 test byte [V_BTYP],8
 jnz .cx
 mov word [V_BST],0             ; einfache Bitmap
 mov ax,[es:di+2]
 mov [V_BN],ax
 mov word [V_BXR],72
 mov word [V_BYR],72
 lea bx,[di+6]
 jmp .hd
.cx:
 mov ax,[es:di+6]
 mov [V_BST],ax
 mov ax,[es:di+8]
 mov [V_BN],ax
 mov ax,[es:di+14]
 or ax,ax
 jz .np
 add ax,di
 mov [V_BPAL],ax
.np:
 mov ax,[es:di+16]
 cmp ax,8
 jae .xr
 mov ax,72
.xr:
 mov [V_BXR],ax
 mov ax,[es:di+18]
 cmp ax,8
 jae .yr
 mov ax,72
.yr:
 mov [V_BYR],ax
 mov bx,[es:di+12]
 add bx,di
.hd:
 cmp word [V_BN],0
 jne .go
 ret
.go:
 push bx                        ; Datenzeiger
 ; Zeilenlaenge
 mov ax,[V_BW]
 add ax,7
 mov cl,3
 shr ax,cl
 mov dx,ax                      ; Bytes je Zeile bei 1 Bit
 xor cx,cx
 test byte [V_BTYP],0x10
 jz .nm
 mov cx,dx
.nm:
 mov [V_BMK],cx
 mov al,[V_BFMT]
 or al,al
 jz .rl
 mov dx,[V_BW]
 cmp al,1
 jne .r8
 inc dx
 shr dx,1
 jmp .rl
.r8:
 cmp al,3
 jne .rl
 mov ax,dx
 add dx,ax
 add dx,ax
.rl:
 add dx,[V_BMK]
 mov [V_BRL],dx
 ; Matrix: GState-Transformation, dann Bild-Einheitsquadrat
 OUT "q "
 mov bx,V_OM
 mov cx,6
.om:
 push cx
 mov cx,[bx]
 mov dx,[bx+2]
 call out_fix
 OUT " "
 add bx,4
 pop cx
 loop .om
 OUT "cm "
 mov ax,[V_BW]
 mov bx,[V_BXR]
 call bm_ratio
 call out_fix
 OUT " 0 0 "
 mov ax,[V_BN]
 mov bx,[V_BYR]
 call bm_ratio
 NEG32 dx,cx
 call out_fix
 OUT " "
 mov ax,[V_BX]
 call out_int
 OUT " "
 mov ax,[V_BST]
 add ax,[V_BN]
 mov bx,[V_BYR]
 call bm_ratio
 add dx,[V_BY]
 call out_fix
 OUT " cm",10
 ; Bildkopf
 cmp byte [V_BFMT],0
 jne .col
 mov bx,V_AC                    ; 1 Bit: Flaechenfarbe, Weiss wird Schwarz
 mov al,[bx]
 and al,[bx+1]
 and al,[bx+2]
 cmp al,255
 jne .ac
 OUT "0 g "
 jmp .acd
.ac:
 call out_rgb
 OUT " rg "
.acd:
 OUT "BI /W "
 mov ax,[V_BW]
 call out_int
 OUT " /H "
 mov ax,[V_BN]
 call out_int
 OUT " /IM true /D [1 0] /F /AHx ID",10
 jmp .data
.col:
 OUT "BI /W "
 mov ax,[V_BW]
 call out_int
 OUT " /H "
 mov ax,[V_BN]
 call out_int
 cmp byte [V_BFMT],3
 jne .ix
 OUT " /BPC 8 /CS /RGB /F /AHx ID",10
 jmp .data
.ix:
 mov cx,16
 cmp byte [V_BFMT],1
 jne .b8
 OUT " /BPC 4"
 jmp .p16
.b8:
 mov cx,256
 OUT " /BPC 8"
.p16:
 OUT " /CS [/I /RGB "
 mov ax,cx
 dec ax
 call out_int
 OUT " <"
 call bm_palette
 OUT ">] /F /AHx ID",10
.data:
 pop bx
 call bm_data
 OUT ">",10,"EI Q",10
 ret

; CX Eintraege Farbtabelle als Hex ausgeben (aus der Bitmap oder dem System)
bm_palette:
 xor dx,dx
.l:
 mov bx,[V_BPAL]
 or bx,bx
 jz .sys
 mov ax,dx                      ; Eintrag DX der mitgelieferten Tabelle
 add bx,ax
 add bx,ax
 add bx,ax
 lea ax,[bx+3]
 cmp ax,[V_BEND]
 ja .sys
 mov al,[es:bx]
 call out_hex2
 mov al,[es:bx+1]
 call out_hex2
 mov al,[es:bx+2]
 call out_hex2
 jmp .n
.sys:
 push cx
 push dx
 push es
 push si
 mov ah,dl
 xor al,al
 xor di,di
 KCALL 247                      ; GrMapColorIndex -> AL R, BL G, BH B
 DSDG
 pop si
 pop es
 call out_hex2
 mov al,bl
 call out_hex2
 mov al,bh
 call out_hex2
 pop dx
 pop cx
.n:
 inc dx
 cmp dx,cx
 jb .l
 ret

; Bilddaten ab ES:BX (ungepackt oder PackBits), Maskenbytes je Zeile auslassen
bm_data:
 mov ax,[V_BN]
 mov [V_BROWS],ax
 mov word [V_BPOS],0
 mov word [V_BLN],0
.l:
 cmp word [V_BROWS],0
 je .x
 cmp bx,[V_BEND]
 jae .fill
 mov al,[es:bx]
 inc bx
 cmp byte [V_BCMP],1
 je .pk
 call bm_put                    ; ungepackt
 jmp .l
.pk:                            ; PackBits: AL = Steuerbyte
 cmp al,128
 je .l
 ja .rep
 xor ah,ah
 inc ax
 mov cx,ax
.lit:
 cmp bx,[V_BEND]
 jae .fill
 mov al,[es:bx]
 inc bx
 call bm_put
 loop .lit
 jmp .l
.rep:
 xor ah,ah
 mov cx,257
 sub cx,ax
 cmp bx,[V_BEND]
 jae .fill
 mov al,[es:bx]
 inc bx
.r:
 call bm_put
 loop .r
 jmp .l
.fill:                          ; fehlende Daten mit 0 auffuellen
 cmp word [V_BROWS],0
 je .x
 xor al,al
 call bm_put
 jmp .fill
.x:
 ret

; AL = Datenbyte der Bitmap. Maskenbytes am Zeilenanfang werden uebersprungen.
bm_put:
 cmp word [V_BROWS],0
 je .x
 push ax
 mov ax,[V_BPOS]
 cmp ax,[V_BMK]
 pop ax
 jb .skip
 call out_hex2
 inc word [V_BLN]
 cmp word [V_BLN],40
 jb .skip
 mov word [V_BLN],0
 push ax
 mov al,10
 call out_byte
 pop ax
.skip:
 inc word [V_BPOS]
 push ax
 mov ax,[V_BPOS]
 cmp ax,[V_BRL]
 pop ax
 jb .x
 mov word [V_BPOS],0
 dec word [V_BROWS]
.x:
 ret
