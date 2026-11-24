; ============================================================ Eingebettete Schriften
; GEOS-Umrissschriften (Nimbus, FONT\*.FNT) werden als PDF-Type3-Schriften
; eingebettet: nur die benutzten Zeichen, je Zeichen ein Glyphenstrom aus den
; Nimbus-Befehlen (move/line/bezier, relativ, Akzent-Komposita).
; Fehlender Stil (fett/kursiv) wird nachgebildet: kursiv ueber Schraegung der
; Textmatrix, fett ueber Fuellen plus Umriss (B) im Glyphenstrom.

FIDX     equ 0x0a40             ; 128 x 16 Byte: FontID, DOS-Name (13)
FIDX_MAX equ 128
SLOTS    equ 0x1240             ; 8 Schrift-Plaetze je 200h Byte
SLOT_SZ  equ 0x200
NSLOT    equ 8
GBUF     equ 0x2240             ; Lesepuffer (auch DTA beim Suchen)
GBUF_SZ  equ 0x0300

S_ID     equ 0x00               ; FontID
S_STY    equ 0x02               ; gewuenschter Stil (Byte, 30h-Maske)
S_FL     equ 0x03               ; 1 gueltig, 2 fett nachbilden, 4 kursiv nachbilden
S_OBJ    equ 0x04               ; Objektnummer des Schriftobjekts
S_NUSED  equ 0x06               ; Anzahl benutzter Zeichen
S_USED   equ 0x08               ; 28 Byte Bitfeld fuer Zeichen 32..255
S_FIX    equ 0x24               ; Zeiger auf FIDX-Eintrag
S_FIRST  equ 0x26
S_LAST   equ 0x27
S_P1     equ 0x28               ; Dateiposition Teil 1 (dd)
S_P2     equ 0x2c               ; Teil 2 (dd)
S_P1S    equ 0x30
S_P2S    equ 0x32
S_W      equ 0x34               ; 224 Breiten (1/1000 em)

; ------------------------------------------------------------ Hilfen
; Aktueller Platz (V_FNUM >= 15) -> SI
cur_slot:
 push ax
 push cx
 mov si,[V_FNUM]
 sub si,15
 mov cl,9
 shl si,cl
 add si,SLOTS
 pop cx
 pop ax
 ret

; V_PATH = "FONT\" + Name ab DS:SI (asciiz)
fs_mkpath:
 push ax
 push si
 push di
 mov di,V_PATH
 mov word [di],'FO'
 mov word [di+2],'NT'
 mov byte [di+4],'\'
 add di,5
 mov cx,13
.c:
 lodsb
 mov [di],al
 inc di
 or al,al
 jz .e
 loop .c
 mov byte [di],0
.e:
 pop di
 pop si
 pop ax
 ret

; Datei des FIDX-Eintrags DI oeffnen -> BX Handle, CF bei Fehler
font_open:
 push ax
 push cx
 push dx
 push si
 lea si,[di+2]
 call fs_mkpath
 KCALL 73                       ; FilePushDir
 DSDG
 xor ax,ax
 KCALL 76                       ; FileSetStandardPath(Top)
 DSDG
 mov dx,V_PATH
 mov al,0xc0
 KCALL 52                       ; FileOpen
 DSDG
 pushf
 push ax
 KCALL 74                       ; FilePopDir
 DSDG
 pop bx
 popf
 pop si
 pop dx
 pop cx
 pop ax
 ret

; BX = Datei, [V_RP] = Position, DX = Puffer, CX = Anzahl
rd_at:
 push ax
 push bx
 push cx
 push dx
 mov cx,[V_RP+2]
 mov dx,[V_RP]
 xor al,al
 KCALL 60                       ; FilePos
 DSDG
 pop dx
 pop cx
 pop bx
 push bx
 xor al,al
 KCALL 58                       ; FileRead
 DSDG
 pop bx
 pop ax
 ret

; ------------------------------------------------------------ Schriftverzeichnis
; Einmal je Auftrag: FONT\*.FNT durchsuchen (DOS-Suche), FontID je Datei merken.
font_scan:
 cmp word [V_FSCAN],0
 je .go
 ret
.go:
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 push es
 mov word [V_FSCAN],1
 mov word [V_FCNT],0
 KCALL 73                       ; FilePushDir
 DSDG
 xor ax,ax
 KCALL 76                       ; FileSetStandardPath(Top)
 DSDG
 mov ah,0x2f                    ; DTA sichern
 int 0x21
 mov [V_DTAO],bx
 mov [V_DTAS],es
 mov dx,GBUF
 mov ah,0x1a
 int 0x21
 mov di,V_PATH
 mov si,fnt_pat
.pc:
 mov al,[cs:si]
 mov [di],al
 inc si
 inc di
 or al,al
 jnz .pc
 mov dx,V_PATH
 xor cx,cx
 mov ah,0x4e                    ; FindFirst
 int 0x21
.l:
 jc .done
 cmp word [V_FCNT],FIDX_MAX
 jae .done
 mov si,GBUF+0x1e
 call fs_mkpath
 mov dx,V_PATH
 mov al,0xc0
 KCALL 52                       ; FileOpen
 DSDG
 jc .nx
 mov bx,ax
 mov dx,GBUF+0x40
 mov cx,10
 xor al,al
 push bx
 KCALL 58                       ; FileRead
 DSDG
 pop bx
 xor al,al
 KCALL 54                       ; FileClose
 DSDG
 cmp word [GBUF+0x40],'BS'
 jne .nx
 cmp word [GBUF+0x42],'WF'
 jne .nx
 mov di,[V_FCNT]
 mov cl,4
 shl di,cl
 add di,FIDX
 mov ax,[GBUF+0x48]
 mov [di],ax
 mov si,GBUF+0x1e
 lea bx,[di+2]
 mov cx,13
.cn:
 mov al,[si]
 mov [bx],al
 inc si
 inc bx
 loop .cn
 inc word [V_FCNT]
.nx:
 mov dx,GBUF                    ; DTA koennte verstellt sein
 mov ah,0x1a
 int 0x21
 mov ah,0x4f                    ; FindNext
 int 0x21
 jmp .l
.done:
 push ds
 mov dx,[V_DTAO]
 mov ds,[V_DTAS]
 mov ah,0x1a
 int 0x21
 pop ds
 KCALL 74                       ; FilePopDir
 DSDG
 pop es
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 ret
fnt_pat: db "FONT\*.FNT",0

; ------------------------------------------------------------ Schrift-Platz
; AX = FontID, BL = Stil (30h-Maske) -> BX = Platznummer, CF = 1 wenn keine Datei
slot_get:
 push cx
 push dx
 push si
 push di
 xor cx,cx
 mov si,SLOTS
.s:
 cmp cx,[V_NSL]
 jae .new
 cmp [si+S_ID],ax
 jne .n
 cmp [si+S_STY],bl
 jne .n
 test byte [si+S_FL],1
 jz .fail
 jmp .ok
.n:
 add si,SLOT_SZ
 inc cx
 jmp .s
.new:
 cmp cx,NSLOT
 jae .fail
 inc word [V_NSL]
 mov [si+S_ID],ax
 mov [si+S_STY],bl
 mov byte [si+S_FL],0
 mov word [si+S_NUSED],0
 push cx
 lea di,[si+S_USED]
 mov cx,14
.z:
 mov word [di],0
 add di,2
 loop .z
 call slot_load
 pop cx
 test byte [si+S_FL],1
 jz .fail
.ok:
 mov bx,cx
 clc
 jmp .r
.fail:
 stc
.r:
 pop di
 pop si
 pop dx
 pop cx
 ret

; SI = neuer Platz (S_ID, S_STY gesetzt): Datei suchen, Kopf und Breiten lesen
slot_load:
 push ax
 push bx
 push cx
 push dx
 push di
 call font_scan
 mov ax,[si+S_ID]
 mov cx,[V_FCNT]
 mov di,FIDX
.f:
 jcxz .xj
 cmp [di],ax
 je .fd
 add di,16
 dec cx
 jmp .f
.xj:
 jmp .x
.fd:
 mov [si+S_FIX],di
 call font_open
 jc .xj
 mov [V_FHF],bx
 mov word [V_RP],0
 mov word [V_RP+2],0
 mov dx,GBUF
 mov cx,512
 call rd_at
 ; Umrisstabelle: Eintraege zu 26 Byte (Stil, Gewicht, 3 x OutlineEntry)
 mov di,[GBUF+37]
 add di,6+GBUF
 mov dx,[GBUF+39]
 add dx,6+GBUF
 cmp dx,GBUF+512
 jbe .lim
 mov dx,GBUF+512
.lim:
 cmp di,dx
 jb .hasol
 jmp .cl                        ; keine Umrisse (Bitmap-Schrift)
.hasol:
 mov bx,di                      ; Vorgabe: erster Eintrag
 mov al,[si+S_STY]
 push di
.e1:                            ; 1. Wahl: Stil passt genau
 cmp di,dx
 jae .e2
 mov ah,[di]
 and ah,0x30
 cmp ah,al
 je .take
 add di,26
 jmp .e1
.e2:                            ; 2. Wahl: Grundstil
 pop di
 push di
.e3:
 cmp di,dx
 jae .e4
 test byte [di],0x30
 jz .take
 add di,26
 jmp .e3
.e4:
 mov di,bx
.take:
 pop bx
 ; nachzubildende Stile
 mov ah,[di]
 not ah
 and ah,al                      ; gewuenscht, aber nicht vorhanden
 mov cl,1
 test ah,0x20
 jz .nb
 or cl,2
.nb:
 test ah,0x10
 jz .ni
 or cl,4
.ni:
 mov [V_SLFL],cl
 mov ax,[di+10]                 ; Teil 1
 mov [si+S_P1],ax
 mov ax,[di+12]
 mov [si+S_P1+2],ax
 mov ax,[di+14]
 mov [si+S_P1S],ax
 mov ax,[di+18]                 ; Teil 2
 mov [si+S_P2],ax
 mov ax,[di+20]
 mov [si+S_P2+2],ax
 mov ax,[di+22]
 mov [si+S_P2S],ax
 mov ax,[di+2]                  ; Kopf
 mov [V_RP],ax
 mov ax,[di+4]
 mov [V_RP+2],ax
 mov cx,[di+6]
 cmp cx,GBUF_SZ
 jbe .hs
 mov cx,GBUF_SZ
.hs:
 mov bx,[V_FHF]
 mov dx,GBUF
 call rd_at
 mov al,[GBUF+22]
 mov [si+S_FIRST],al
 mov al,[GBUF+23]
 mov [si+S_LAST],al
 ; Breiten fuer 32..255
 xor cx,cx                      ; CL = Zeichen - 32
 lea di,[si+S_W]
.w:
 xor ax,ax
 mov dl,cl
 add dl,32
 cmp dl,[si+S_FIRST]
 jb .ws
 cmp dl,[si+S_LAST]
 ja .ws
 sub dl,[si+S_FIRST]
 xor dh,dh
 cmp dx,[GBUF+31]               ; numChars
 jae .ws
 mov bx,dx
 add bx,dx
 add bx,dx
 mov ax,[GBUF+43+bx]
.ws:
 mov [di],ax
 add di,2
 inc cx
 cmp cx,224
 jb .w
 mov al,[V_SLFL]
 mov [si+S_FL],al
.cl:
 mov bx,[V_FHF]
 xor al,al
 KCALL 54                       ; FileClose
 DSDG
.x:
 pop di
 pop dx
 pop cx
 pop bx
 pop ax
 ret

; ------------------------------------------------------------ waehrend der Seite
; AL = Zeichen: in der aktuellen eingebetteten Schrift als benutzt markieren
mark_used:
 cmp word [V_FNUM],15
 jb .x
 cmp al,32
 jb .x
 push ax
 push bx
 push cx
 push si
 call cur_slot
 mov bl,al
 sub bl,32
 xor bh,bh
 mov cl,bl
 and cl,7
 mov ch,1
 shl ch,cl
 shr bx,1
 shr bx,1
 shr bx,1
 test [si+S_USED+bx],ch
 jnz .p
 or [si+S_USED+bx],ch
 inc word [si+S_NUSED]
.p:
 pop si
 pop cx
 pop bx
 pop ax
.x:
 ret

; DX:CX += Schraegung * [BX], wenn die aktuelle Schrift kursiv nachgebildet wird
skew_add:
 cmp word [V_FNUM],15
 jb .x
 push si
 call cur_slot
 test byte [si+S_FL],4
 pop si
 jz .x
 push ax
 push dx
 push cx
 mov cx,[bx]
 mov dx,[bx+2]
 push bx
 mov bx,V_SHEAR
 call mulfix
 pop bx
 pop ax
 add cx,ax
 pop ax
 adc dx,ax
 pop ax
.x:
 ret

; ------------------------------------------------------------ Auftragsende
; Objektnummern vergeben: ab AX; je Schrift 1 + 2 je Zeichen. -> AX = naechste
fonts_alloc:
 push cx
 push si
 xor cx,cx
 mov si,SLOTS
.l:
 cmp cx,[V_NSL]
 jae .x
 test byte [si+S_FL],1
 jz .n
 cmp word [si+S_NUSED],0
 je .n
 mov [si+S_OBJ],ax
 inc ax
 add ax,[si+S_NUSED]
 add ax,[si+S_NUSED]
.n:
 add si,SLOT_SZ
 inc cx
 jmp .l
.x:
 pop si
 pop cx
 ret

; Eintraege fuer das Schrift-Ressourcenobjekt: "/F15 n 0 R" ...
fonts_res:
 push ax
 push cx
 push si
 xor cx,cx
 mov si,SLOTS
.l:
 cmp cx,[V_NSL]
 jae .x
 test byte [si+S_FL],1
 jz .n
 cmp word [si+S_NUSED],0
 je .n
 OUT "/F"
 mov ax,cx
 add ax,15
 call out_int
 OUT " "
 mov ax,[si+S_OBJ]
 call out_int
 OUT " 0 R",10
.n:
 add si,SLOT_SZ
 inc cx
 jmp .l
.x:
 pop si
 pop cx
 pop ax
 ret

; Alle benutzten eingebetteten Schriften schreiben
fonts_write:
 push cx
 push si
 xor cx,cx
 mov si,SLOTS
.l:
 cmp cx,[V_NSL]
 jae .x
 test byte [si+S_FL],1
 jz .n
 cmp word [si+S_NUSED],0
 je .n
 call font_write
.n:
 add si,SLOT_SZ
 inc cx
 jmp .l
.x:
 pop si
 pop cx
 ret

; SI = Platz
font_write:
 push ax
 push bx
 push cx
 push dx
 push di
 mov word [V_FHF],0xffff
 mov di,[si+S_FIX]
 call font_open
 jc .nf
 mov [V_FHF],bx
.nf:
 mov ax,[si+S_OBJ]
 call obj_start
 OUT "<< /Type /Font /Subtype /Type3 /FontBBox [-300 -500 1500 1300]"
 OUT " /FontMatrix [0.001 0 0 0.001 0 0] /Resources << >>",10
 OUT "/FirstChar 32 /LastChar 255 /Widths ["
 xor cx,cx
.w:
 mov bx,cx
 shl bx,1
 mov ax,[si+S_W+bx]
 call out_int
 OUT " "
 inc cx
 cmp cx,224
 jb .w
 OUT "]",10,"/Encoding << /Type /Encoding /Differences ["
 mov cx,32
.d:
 call is_used
 jz .dn
 mov ax,cx
 call out_int
 OUT " /"
 call out_gname
 OUT " "
.dn:
 inc cx
 cmp cx,256
 jb .d
 OUT "] >>",10,"/CharProcs <<"
 mov dx,[si+S_OBJ]
 inc dx
 mov cx,32
.p:
 call is_used
 jz .pn
 OUT " /"
 call out_gname
 OUT " "
 mov ax,dx
 call out_int
 OUT " 0 R"
 add dx,2
.pn:
 inc cx
 cmp cx,256
 jb .p
 OUT " >> >>",10,"endobj",10
 ; Glyphen
 mov dx,[si+S_OBJ]
 inc dx
 mov cx,32
.g:
 call is_used
 jz .gn
 call glyph_obj                 ; CL = Zeichen, DX = Objektnummer
 add dx,2
.gn:
 inc cx
 cmp cx,256
 jb .g
 mov bx,[V_FHF]
 cmp bx,0xffff
 je .x
 xor al,al
 KCALL 54                       ; FileClose
 DSDG
.x:
 pop di
 pop dx
 pop cx
 pop bx
 pop ax
 ret

; CL = Zeichen, SI = Platz -> ZF = 0 wenn benutzt
is_used:
 push ax
 push bx
 push cx
 mov bl,cl
 sub bl,32
 xor bh,bh
 and cl,7
 mov al,1
 shl al,cl
 shr bx,1
 shr bx,1
 shr bx,1
 test [si+S_USED+bx],al
 pop cx
 pop bx
 pop ax
 ret

; CL = Zeichen, DX = Objektnummer (Strom), DX+1 = Laenge
glyph_obj:
 push ax
 push bx
 push cx
 push dx
 mov ax,dx
 call obj_start
 OUT "<< /Length "
 mov ax,dx
 inc ax
 call out_int
 OUT " 0 R >>",10,"stream",10
 mov ax,[V_POS]
 mov [V_SSTART],ax
 mov ax,[V_POS+2]
 mov [V_SSTART+2],ax
 call glyph_emit
 mov ax,[V_POS]
 mov bx,[V_POS+2]
 sub ax,[V_SSTART]
 sbb bx,[V_SSTART+2]
 push bx
 push ax
 OUT 10,"endstream",10,"endobj",10
 pop ax
 pop bx
 pop dx
 push dx
 push bx
 push ax
 mov ax,dx
 inc ax
 call obj_start
 pop ax
 pop dx
 push si
 xor si,si
 call out_u32
 pop si
 OUT 10,"endobj",10
 pop dx
 pop cx
 pop bx
 pop ax
 ret

; CL = Zeichen: Glyphenbeschreibung ausgeben
glyph_emit:
 push ax
 push bx
 push cx
 push dx
 mov bl,cl
 sub bl,32
 xor bh,bh
 shl bx,1
 mov ax,[si+S_W+bx]
 mov [V_GW],ax
 mov al,cl
 call glyph_load
 jnc .ok
 mov ax,[V_GW]                  ; kein Umriss: nur Breite
 call out_int
 OUT " 0 d0",10
 jmp .x
.ok:
 mov ax,[V_GW]
 call out_int
 OUT " 0 "
 mov ax,[V_BB]
 call out_int
 OUT " "
 mov ax,[V_BB+2]
 call out_int
 OUT " "
 mov ax,[V_BB+4]
 call out_int
 OUT " "
 mov ax,[V_BB+6]
 call out_int
 OUT " d1",10
 test byte [si+S_FL],2
 jz .nb
 OUT "40 w 1 j "
.nb:
 mov word [V_GDX],0
 mov word [V_GDY],0
 call glyph_path                ; CF = 1: Kompositum in V_GC1/V_GC2/V_GX/V_GY
 jnc .fill
 mov al,[V_GC1]
 call glyph_load
 jc .c2
 call glyph_path
.c2:
 mov ax,[V_GX]
 mov [V_GDX],ax
 mov ax,[V_GY]
 mov [V_GDY],ax
 mov al,[V_GC2]
 call glyph_load
 jc .fill
 call glyph_path
.fill:
 test byte [si+S_FL],2
 jz .f
 OUT "B",10
 jmp .x
.f:
 OUT "f",10
.x:
 pop dx
 pop cx
 pop bx
 pop ax
 ret

; AL = Zeichen: Lesestrom auf die Zeichendaten setzen und die Begrenzung
; (4 Woerter) nach V_BB lesen. CF = 1 wenn das Zeichen keine Daten hat.
glyph_load:
 push ax
 push bx
 push cx
 push dx
 cmp word [V_FHF],0xffff
 je .no
 cmp al,[si+S_FIRST]
 jb .no
 cmp al,[si+S_LAST]
 ja .no
 xor ah,ah
 cmp al,0x80
 jae .p2
 sub al,[si+S_FIRST]
 mov bx,S_P1
 mov dx,[si+S_P1S]
 jmp .pt
.p2:
 sub al,0x80
 mov bx,S_P2
 mov dx,[si+S_P2S]
.pt:
 mov [V_GPS],dx
 shl ax,1
 mov cx,[si+bx]
 mov dx,[si+bx+2]
 mov [V_GPB],cx
 mov [V_GPB+2],dx
 add cx,ax
 adc dx,0
 mov [V_RP],cx
 mov [V_RP+2],dx
 mov bx,[V_FHF]
 mov word [GBUF],0
 mov dx,GBUF
 mov cx,2
 call rd_at
 mov ax,[GBUF]                  ; Offset im Teil
 or ax,ax
 jz .no
 cmp ax,[V_GPS]
 jae .no
 mov cx,[V_GPS]
 sub cx,ax
 mov [V_SREM],cx                ; Bytes bis zum Ende des Teils
 mov dx,[V_GPB+2]
 add ax,[V_GPB]
 adc dx,0
 mov [V_SPOS],ax
 mov [V_SPOS+2],dx
 mov word [V_GEND],GBUF         ; Puffer leer
 push si
 mov si,GBUF
 call gw
 mov [V_BB],ax
 call gw
 mov [V_BB+2],ax
 call gw
 mov [V_BB+4],ax
 call gw
 mov [V_BB+6],ax
 mov [V_SPTR],si
 pop si
 clc
 jmp .x
.no:
 stc
.x:
 pop dx
 pop cx
 pop bx
 pop ax
 ret

; Naechstes Byte des Zeichenstroms -> AL (DS:SI = Lesezeiger in GBUF).
; Am Ende der Daten wird 3 (NIMBUS_DONE) geliefert.
gb:
 cmp si,[V_GEND]
 jb .h
 push bx
 push cx
 push dx
 mov cx,[V_SREM]
 cmp cx,GBUF_SZ
 jbe .c
 mov cx,GBUF_SZ
.c:
 jcxz .e
 sub [V_SREM],cx
 mov ax,[V_SPOS]
 mov [V_RP],ax
 mov ax,[V_SPOS+2]
 mov [V_RP+2],ax
 add [V_SPOS],cx
 adc word [V_SPOS+2],0
 mov bx,[V_FHF]
 mov dx,GBUF
 call rd_at
 mov si,GBUF
 mov ax,GBUF
 add ax,cx
 mov [V_GEND],ax
 pop dx
 pop cx
 pop bx
.h:
 lodsb
 ret
.e:
 pop dx
 pop cx
 pop bx
 mov al,3
 ret
; Naechstes Wort -> AX
gw:
 call gb
 push ax
 call gb
 mov ah,al
 pop bx
 mov al,bl
 ret

; Punkt AX, BX (+ V_GDX/V_GDY) ausgeben
gpt:
 push ax
 add ax,[V_GDX]
 call out_int
 OUT " "
 mov ax,bx
 add ax,[V_GDY]
 call out_int
 OUT " "
 pop ax
 ret

; Nimbus-Befehle aus dem Zeichenstrom als Pfad ausgeben.
; CF = 1 bei Akzent-Kompositum (V_GC1, V_GX, V_GY, V_GC2 gesetzt).
glyph_path:
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 mov si,[V_SPTR]
 mov di,2
.t:
 call gb                        ; Tripel ueberspringen
 xor ah,ah
 mov cx,ax
 add cx,ax
 add cx,ax
 add cx,cx
 jcxz .tn
.ts:
 call gb
 loop .ts
.tn:
 dec di
 jnz .t
 xor cx,cx                      ; CX, DX = aktueller Punkt
 xor dx,dx
.l:
 call gb
 cmp al,0
 je .mv
 cmp al,1
 je .ln
 cmp al,2
 je .bz
 cmp al,5
 je .acc
 cmp al,6
 je .vl
 cmp al,7
 je .hl
 cmp al,8
 je .rl
 cmp al,9
 je .rc
 jmp .done                      ; 3 = Ende (oder unbekannt)
.mv:
 call gw
 mov cx,ax
 call gw
 mov dx,ax
 mov ax,cx
 mov bx,dx
 call gpt
 OUT "m "
 jmp .l
.ln:
 call gw
 mov cx,ax
 call gw
 mov dx,ax
.lout:
 mov ax,cx
 mov bx,dx
 call gpt
 OUT "l "
 jmp .l
.bz:
 mov di,3
.bp:
 call gw
 mov cx,ax
 call gw
 mov dx,ax
 mov bx,dx
 mov ax,cx
 call gpt
 dec di
 jnz .bp
 OUT "c "
 jmp .l
.vl:
 call gw
 add dx,ax
 jmp .lout
.hl:
 call gw
 add cx,ax
 jmp .lout
.rl:
 call gb
 cbw
 add dx,ax
 call gb
 cbw
 add cx,ax
 jmp .lout
.rc:
 mov di,3
.rp:
 call gb
 cbw
 add dx,ax
 call gb
 cbw
 add cx,ax
 mov ax,cx
 mov bx,dx
 call gpt
 dec di
 jnz .rp
 OUT "c "
 jmp .l
.acc:
 call gb
 mov [V_GC1],al
 call gw
 mov [V_GX],ax
 call gw
 mov [V_GY],ax
 call gb
 mov [V_GC2],al
 OUT 10
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 stc
 ret
.done:
 OUT 10
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 clc
 ret

; CL = Zeichen: Glyphenname (Adobe-Liste) ausgeben
out_gname:
 push ax
 push bx
 mov bl,cl
 sub bl,32
 xor bh,bh
 shl bx,1
 mov bx,[cs:gn_off+bx]
.l:
 mov al,[cs:bx]
 or al,al
 jz .x
 call out_byte
 inc bx
 jmp .l
.x:
 pop bx
 pop ax
 ret

%include "gnames.inc"
