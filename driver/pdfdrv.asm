; SPDX-License-Identifier: MIT
; PDF-Druckertreiber fuer GeoWorks Ensemble 1.2 (Meilenstein 0)
;
; Der Spooler behandelt den Treiber wie PSCRIPT.GEO (Smarts = PDL): pro Seite
; START_PAGE, ESC 8100h (Seitentransformation), ESC 8101h (GString), END_PAGE.
; Ausgabe nur bei "In Datei drucken" (Porttyp 4): PDF 1.3 in das Datei-Handle,
; das der Spooler mit SET_STREAM uebergibt.
;
; Uebersetzt: Linie, H/V-Linie, Rechteck, Fuellrechteck, Polylinie, Polygon,
; Fuellpolygon, Ellipse, Fuellellipse, Text, Text an CP, Zeichen.
; Alle anderen Ausgabe-Opcodes erscheinen als Kommentar "% op XX" im PDF.
; Kernel-Nummern: doc/KERNEL12.md, Opcodes: doc/GSTRING12.md.

BITS 16
CPU 8086
ORG 0
section .text start=0 vstart=0 align=1
section .relocs follows=.text align=1
section .text

; ------------------------------------------------------------ Makros
%macro KCALL 1                 ; Kernel-Einsprung (Relocation 00h)
%%c: call 0:%1
 section .relocs
 db 0x00,0
 dw %%c+1
 section .text
%endmacro
%macro RSEG 2                  ; mov reg16, Segment der Ressource %2
%%c: mov %1, strict word %2
 section .relocs
 db 0x22,0
 dw %%c+1
 section .text
%endmacro
%macro RHANDLE 2               ; mov reg16, Handle der Ressource %2
%%c: mov %1, strict word %2
 section .relocs
 db 0x23,0
 dw %%c+1
 section .text
%endmacro
%macro DSDG 0                  ; DS = DGROUP
 push ax
 RSEG ax,1
 mov ds,ax
 pop ax
%endmacro
%macro OUT 1+                  ; Text ins PDF
 call out_inl
 db %1,0
%endmacro

; ------------------------------------------------------------ DGROUP
DG_SIZE  equ 0x38a0
V_FH     equ 0x10
V_PORT   equ 0x12
V_NOFILE equ 0x14
V_PW     equ 0x16
V_PH     equ 0x18
V_POS    equ 0x1a
V_BUFLEN equ 0x1e
V_PAGES  equ 0x20
V_INPAGE equ 0x22
V_SBLK   equ 0x24
V_SCHK   equ 0x26
V_G      equ 0x28
V_SRC    equ 0x2a
V_RET    equ 0x2c
V_SSTART equ 0x2e
V_LC     equ 0x32
V_AC     equ 0x35
V_TC     equ 0x38
V_LW     equ 0x3c
V_FSIZE  equ 0x3e
V_CPX    equ 0x40
V_CPY    equ 0x42
V_SSEG   equ 0x44
V_FILL   equ 0x52
V_TM     equ 0x60
V_ITER   equ 0x7a
V_XREF   equ 0x7c
V_OM     equ 0x80          ; Objektmatrix (24 Byte)
V_PX     equ 0x98
V_PY     equ 0x9c
V_QX     equ 0xa0
V_QY     equ 0xa4
V_ECX    equ 0xa8
V_ECY    equ 0xac
V_ERX    equ 0xb0
V_ERY    equ 0xb4
V_EKX    equ 0xb8
V_EKY    equ 0xbc
V_KAP    equ 0xc0
V_ELEN   equ 0xc4
V_TS     equ 0xc6
V_TFN    equ 0xc8
V_TFR    equ 0xca
V_TFX    equ 0xcc
V_TFY    equ 0xce
V_TFP    equ 0xd0
V_TFC    equ 0xd2
V_TFREM  equ 0xd4
V_RGB    equ 0xd6
V_FID    equ 0xda
V_STYLE  equ 0xdc
V_FNUM   equ 0xde
V_XB     equ 0x5c0
V_X0     equ 0x5c4
V_XM     equ 0x5c8
V_Y0     equ 0x5cc
V_Y1     equ 0x5d0
V_YA     equ 0x5d4
V_ACC    equ 0x5d8
V_NCH    equ 0x5dc
V_TC2    equ 0x5de
V_RSZ    equ 0x5e0
V_RSTY   equ 0x5e2
V_TBASE  equ 0x5e4
V_RUN    equ 0x5e6
V_RSH    equ 0x5e8
V_BW     equ 0xa00
V_BN     equ 0xa02
V_BRL    equ 0xa04
V_BMK    equ 0xa06
V_BPOS   equ 0xa08
V_BROWS  equ 0xa0a
V_BLN    equ 0xa0c
V_BX     equ 0xa0e
V_BY     equ 0xa10
V_BFMT   equ 0xa12
V_BEND   equ 0xa14
V_BXR    equ 0xa16
V_BYR    equ 0xa18
V_BST    equ 0xa1a
V_BPAL   equ 0xa1c
V_BTYP   equ 0xa1e
V_BCMP   equ 0xa20
V_INV    equ 0xa22
V_NSL    equ 0x3800
V_FSCAN  equ 0x3802
V_FCNT   equ 0x3804
V_FHF    equ 0x3806
V_SHEAR  equ 0x3808
V_GDX    equ 0x380c
V_GDY    equ 0x380e
V_GC1    equ 0x3810
V_GC2    equ 0x3811
V_DTAO   equ 0x3812
V_DTAS   equ 0x3814
V_OBJN   equ 0x3816
V_GX     equ 0x3818
V_GY     equ 0x381a
V_GW     equ 0x381c
V_GEND   equ 0x381e
V_RP     equ 0x3820
V_GPB    equ 0x3824
V_GPS    equ 0x3828
V_SLFL   equ 0x382a
V_PATH   equ 0x382c              ; 20 Byte (bis 3840h)
V_SREM   equ 0x3840
V_SPOS   equ 0x3842
V_SPTR   equ 0x3846
V_BB     equ 0x3848
V_APAT   equ 0x3850
V_PUSED  equ 0x3852              ; Bitfeld Muster 0..31 (dd)
V_POBJ   equ 0x3856              ; erste Objektnummer der Muster
V_ROT    equ 0x3858
V_NEST   equ 0x3860
V_EBLK   equ 0x3862
V_OUTER  equ 0x3864              ; 24 Byte
V_TA     equ 0x387c              ; 24 Byte
V_OBJ    equ 0x2540              ; 1200 Objekte
MAXOBJ   equ 1200
V_BUF    equ 0x600
BUFSZ    equ 0x400
MAXP     equ 100

; ============================================================ Strategie
; DI = Funktion. Ab DI=6 ist BX das PState-Handle; es wird gesperrt,
; ES = PState-Segment. DS = DGROUP waehrend der Funktion.
strategy:
 push bp
 push bx
 push di
 push ds
 push es
 cmp di,6
 jb .nolock
 push ax
 KCALL 5                       ; MemLock(PState)
 mov bp,ax
 mov es,ax
 pop ax
.nolock:
 DSDG
 call dispatch
 pop es
 pop ds
 pop di
 pop bx
 pushf
 cmp di,6
 jb .nounl
 KCALL 6                       ; MemUnlock(PState)
.nounl:
 popf
 pop bp
 retf

dispatch:
 test di,0x8000
 jnz .esc
 cmp di,0x32
 ja f_nop
 test di,1
 jnz f_nop
 jmp [cs:di+fntab]
.esc:
 cmp di,0x8100
 je esc_settm
 cmp di,0x8101
 je esc_gstring
 jmp f_nop

fntab:
 dw f_nop        ; 00 DR_INIT
 dw f_nop        ; 02 DR_EXIT
 dw f_test       ; 04 Geraet testen
 dw f_setdev     ; 06 Geraet setzen
 dw f_drvinfo    ; 08 Treiber-Info
 dw f_devinfo    ; 0A Geraete-Info
 dw f_setmode    ; 0C Modus setzen
 dw f_setstream  ; 0E Ausgabeziel
 dw f_area       ; 10 Druckbereich
 dw f_getmode    ; 12
 dw f_getstyle   ; 14
 dw f_getcur     ; 16
 dw f_home       ; 18
 dw f_startjob   ; 1A
 dw f_endjob     ; 1C
 dw f_nop        ; 1E
 dw f_nop        ; 20
 dw f_nop        ; 22
 dw f_nop        ; 24 Text (Textmodus, nicht benutzt)
 dw f_zero       ; 26
 dw f_zero       ; 28
 dw f_nop        ; 2A Rohdaten
 dw f_setcur     ; 2C
 dw f_startpage  ; 2E
 dw f_endpage    ; 30
 dw f_nop        ; 32

f_nop:
 clc
 ret
f_zero:
 xor ax,ax
 clc
 ret
f_test:
 xor ax,ax
 clc
 ret
f_setdev:                       ; DX:SI Geraetename; nur ein Geraet
 mov word [es:1],0
 push dx
 RHANDLE dx,3
 mov [es:3],dx
 pop dx
 clc
 ret
f_drvinfo:
 RHANDLE dx,4
 xor si,si
 clc
 ret
f_devinfo:
 mov dx,[es:3]
 or dx,dx
 jnz .ok
 RHANDLE dx,3
.ok:
 xor si,si
 clc
 ret
f_setmode:                      ; CL Modus, DL/DH Papier, AX Breite, SI Hoehe
 mov [es:0],cl
 mov [es:7],dl
 mov [es:8],dh
 mov [es:9],ax
 mov [es:0x0b],si
 mov [V_PW],ax
 mov [V_PH],si
 clc
 ret
f_setstream:                    ; CX Token/Datei, DX Stream-Treiber, SI Porttyp
 mov [es:0x17],cx
 mov [es:0x19],si
 mov [V_FH],cx
 mov [V_PORT],si
 clc
 ret
f_area:                         ; -> AX links, SI oben, CX rechts, DX unten
 xor ax,ax
 xor si,si
 mov cx,[es:9]
 mov dx,[es:0x0b]
 or cx,cx
 jnz .w
 mov cx,595
.w:
 or dx,dx
 jnz .h
 mov dx,842
.h:
 clc
 ret
f_getmode:
 mov cl,[es:0x2a]
 clc
 ret
f_getstyle:
 mov cx,[es:0x28]
 clc
 ret
f_getcur:
 mov cx,[es:0x0d]
 mov dx,[es:0x0f]
 clc
 ret
f_home:
 mov word [es:0x0d],0
 mov word [es:0x0f],0
 clc
 ret
f_setcur:
 mov [es:0x0d],cx
 mov [es:0x0f],dx
 clc
 ret

; ============================================================ Auftrag
f_startjob:
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 xor ax,ax
 mov [V_POS],ax
 mov [V_POS+2],ax
 mov [V_BUFLEN],ax
 mov [V_PAGES],ax
 mov [V_INPAGE],ax
 mov word [V_KAP],36195
 mov word [V_KAP+2],0
 mov word [V_NOFILE],1
 cmp word [V_PORT],4
 jne .nf
 mov [V_NOFILE],ax
.nf:
 cmp word [V_PW],0
 jne .pw
 mov word [V_PW],595
.pw:
 cmp word [V_PH],0
 jne .ph
 mov word [V_PH],842
.ph:
 ; Kopierpuffer fuer GString-Elemente (LMem-Block wie PS.GEO)
 mov ax,0x64
 mov cx,0x6050
 KCALL 0                       ; MemAlloc, gesperrt
 push bx
 mov ds,ax
 xor ax,ax
 mov cx,0x060a
 mov dx,0x10
 KCALL 24                      ; LMemInitHeap
 xor cx,cx
 KCALL 25                      ; LMemAlloc -> AX
 pop bx
 DSDG
 mov [V_SBLK],bx
 mov [V_SCHK],ax
 KCALL 6                       ; MemUnlock
 DSDG
 OUT "%PDF-1.3",10,"%",0xe2,0xe3,0xcf,0xd3,10
 OUT "% GeoWorks 1.2 PDF-Druckertreiber, Version 1.2a",10
 xor ax,ax
 mov [V_NSL],ax
 mov [V_FSCAN],ax
 mov word [V_SHEAR],13933       ; tan 12 Grad = 0,2126
 mov [V_SHEAR+2],ax
 mov word [V_OBJN],4
 mov [V_NEST],ax
 mov [V_PUSED],ax
 mov [V_PUSED+2],ax

 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 clc
 ret

f_endjob:
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 cmp word [V_INPAGE],0
 je .np
 call end_page
.np:
 mov ax,[V_PAGES]               ; eingebettete Schriften hinter den Seiten
 call page_obj
 sub ax,2
 call fonts_alloc
 call pats_alloc
 call write_obj3
 call pats_write
 call fonts_write
 mov ax,1
 call obj_start
 OUT "<< /Type /Catalog /Pages 2 0 R >>",10,"endobj",10
 mov ax,2
 call obj_start
 OUT "<< /Type /Pages /Count "
 mov ax,[V_PAGES]
 call out_int
 OUT " /Kids ["
 xor cx,cx
.kids:
 cmp cx,[V_PAGES]
 jae .kd
 mov ax,cx
 call page_obj                  ; AX = Objektnummer der Seite
 call out_int
 OUT " 0 R "
 inc cx
 jmp .kids
.kd:
 OUT "] >>",10,"endobj",10
 ; xref
 mov ax,[V_POS]
 mov [V_XREF],ax
 mov ax,[V_POS+2]
 mov [V_XREF+2],ax
 OUT "xref",10,"0 "
 call obj_count                 ; AX = hoechste Objektnummer + 1
 call out_int
 OUT 10,"0000000000 65535 f ",10
 mov cx,1
.xr:
 call obj_count
 cmp cx,ax
 jae .xd
 mov bx,cx
 shl bx,1
 shl bx,1
 mov ax,[V_OBJ+bx]
 mov dx,[V_OBJ+bx+2]
 mov si,1                       ; mit Nullen auffuellen
 call out_u32
 OUT " 00000 n ",10
 inc cx
 jmp .xr
.xd:
 OUT "trailer",10,"<< /Size "
 call obj_count
 call out_int
 OUT " /Root 1 0 R >>",10,"startxref",10
 mov ax,[V_XREF]
 mov dx,[V_XREF+2]
 xor si,si
 call out_u32
 OUT 10,"%%EOF",10
 call out_flush
 mov bx,[V_SBLK]
 or bx,bx
 jz .nb
 KCALL 2                       ; MemFree
 DSDG
 mov word [V_SBLK],0
.nb:
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 clc
 ret


; Objekt 3: Schrift-Ressourcen (Standardschriften F1..F14, eingebettete ab F15)
write_obj3:
 push ax
 push cx
 push si
 mov ax,3                       ; Ressourcen: Schriften F1.., Muster P1..
 call obj_start
 OUT "<< /ColorSpace << /CsP [/Pattern /DeviceRGB] >>",10
 call pats_res
 OUT "/Font <<",10
 mov cx,1
 mov si,font_names
.fn:
 OUT "/F"
 mov ax,cx
 call out_int
 OUT " << /Type /Font /Subtype /Type1 /BaseFont /"
.fc:
 mov al,[cs:si]
 inc si
 or al,al
 jz .fe
 call out_byte
 jmp .fc
.fe:
 cmp cx,13                      ; Symbol/ZapfDingbats: eigene Kodierung
 jae .fs
 OUT " /Encoding /MacRomanEncoding"
.fs:
 OUT " >>",10
 inc cx
 cmp cx,15
 jb .fn
 call fonts_res
 OUT ">> >>",10,"endobj",10
 pop si
 pop cx
 pop ax
 ret

; AX=Seitenindex -> AX=Objektnummer der Seite (6+3i)
page_obj:
 push dx
 mov dx,ax
 shl ax,1
 add ax,dx
 add ax,6
 pop dx
 ret

obj_count:                      ; AX = hoechste Objektnummer + 1
 mov ax,[V_OBJN]
 ret

; AX = Objektnummer: Position merken, "n 0 obj" schreiben
obj_start:
 push bx
 push ax
 cmp ax,[V_OBJN]
 jb .k
 inc ax
 mov [V_OBJN],ax
 dec ax
.k:
 cmp ax,MAXOBJ
 jb .in
 pop ax
 push ax
 jmp .wr
.in:
 mov bx,ax
 shl bx,1
 shl bx,1
 push ax
 mov ax,[V_POS]
 mov [V_OBJ+bx],ax
 mov ax,[V_POS+2]
 mov [V_OBJ+bx+2],ax
 pop ax
.wr:
 call out_int
 OUT " 0 obj",10
 pop ax
 pop bx
 ret

; ============================================================ Seite
f_startpage:
 push ax
 push bx
 push dx
 cmp word [V_PAGES],MAXP
 jae .skip
 mov word [V_INPAGE],1
 mov word [V_ROT],0
 mov ax,[V_PAGES]
 call page_obj
 sub ax,2                       ; Inhalt = 4+3i
 push ax
 call obj_start
 OUT "<< /Length "
 pop ax
 inc ax
 call out_int
 OUT " 0 R >>",10,"stream",10
 mov ax,[V_POS]
 mov [V_SSTART],ax
 mov ax,[V_POS+2]
 mov [V_SSTART+2],ax
 OUT "q 1 0 0 -1 0 "
 mov ax,[V_PH]
 call out_int
 OUT " cm",10
.skip:
 pop dx
 pop bx
 pop ax
 clc
 ret

f_endpage:
 cmp word [V_INPAGE],0
 je .x
 call end_page
.x:
 clc
 ret

end_page:
 push ax
 push bx
 push cx
 push dx
 push si
 OUT "Q",10
 mov ax,[V_POS]
 mov dx,[V_POS+2]
 sub ax,[V_SSTART]
 sbb dx,[V_SSTART+2]
 push dx
 push ax
 OUT 10,"endstream",10,"endobj",10
 mov ax,[V_PAGES]
 call page_obj
 dec ax                         ; Laenge = 5+3i
 call obj_start
 pop ax
 pop dx
 xor si,si
 call out_u32
 OUT 10,"endobj",10
 mov ax,[V_PAGES]
 call page_obj
 call obj_start
 OUT "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 "
 mov ax,[V_PW]
 call out_int
 OUT " "
 mov ax,[V_PH]
 call out_int
 OUT "] /Contents "
 mov ax,[V_PAGES]
 call page_obj
 sub ax,2
 call out_int
 OUT " 0 R /Resources 3 0 R"
 mov ax,[V_ROT]
 or ax,ax
 jz .nr
 OUT " /Rotate "
 call out_int
.nr:
 OUT " >>",10,"endobj",10
 inc word [V_PAGES]
 mov word [V_INPAGE],0
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 ret

; ESC 8100h: DX:SI = TransMatrix (6 x WWFixed: Bruch, Ganzzahl)
esc_settm:
 push ax
 push bx
 push cx
 push dx
 push si
 push es
 cmp word [V_INPAGE],0
 je .x
 mov es,dx
 xor bx,bx
.c:
 mov ax,[es:si+bx]
 mov [V_TM+bx],ax
 add bx,2
 cmp bx,24
 jb .c
 xor bx,bx
.o:
 mov cx,[V_TM+bx]
 mov dx,[V_TM+bx+2]
 call out_fix
 OUT " "
 add bx,4
 cmp bx,24
 jb .o
 OUT "cm",10
 ; Querformat: GEOS dreht die Seite um 90 Grad -> /Rotate fuer die Anzeige
 mov word [V_ROT],0
 mov ax,[V_TM+6]                ; e12 ganzzahlig
 or ax,ax
 jz .r0
 mov word [V_ROT],270
 jg .x
 mov word [V_ROT],90
 jmp .x
.r0:
 mov ax,[V_TM+2]                ; e11 ganzzahlig
 or ax,ax
 jns .x
 mov word [V_ROT],180
.x:
 pop es
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 clc
 ret

; ============================================================ GString
; ESC 8101h: SI = GString, CX = Flags. Rueckgabe AX: 1 = Seitenwechsel, 0 = Ende
esc_gstring:
 push bx
 push cx
 push dx
 push si
 push di
 push es
 mov [V_SRC],si
 mov word [V_RET],0
 mov word [V_ITER],0
 xor di,di
 KCALL 211                     ; GrCreateState ohne Fenster
 DSDG
 mov [V_G],di
 call play_loop
 mov di,[V_G]
 KCALL 212                     ; GrDestroyState
 DSDG
 mov ax,[V_RET]
 pop es
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 clc
 ret

; Elemente des GStrings V_SRC einzeln abspielen (Hilfs-GState V_G)
play_loop:
.loop:
 inc word [V_ITER]
 cmp word [V_ITER],8000
 ja .done
 push bp
 mov si,[V_SRC]
 mov di,[V_G]
 xor ax,ax
 xor bx,bx
 xor cx,cx
 xor dx,dx
 mov bp,0x4800                 ; Stopp an Zeichenbefehl und Seitenwechsel
 KCALL 283                     ; GrDrawGString
 mov ax,bp
 pop bp
 DSDG
 cmp ax,0xff
 jne .n1
 xor ax,ax
.n1:
 mov [V_RET],ax
 cmp ax,1
 jbe .done
 cmp word [V_INPAGE],0
 je .skip
 call query_attrs              ; Farben, Breite, Schrift aus G
 call copy_elem                ; ES:SI = Element
 call emit_elem
 call unlock_scratch
 jmp .loop
.skip:
 call copy_elem                ; ohne Seite: nur weiterruecken
 call unlock_scratch
 jmp .loop
.done:
 ret

; Element ueber GrCopyGString in den Kopierpuffer holen (wie PS.GEO).
; Aus: ES:SI = Element, Block gesperrt.
copy_elem:
 mov bx,[V_SBLK]
 mov dx,[V_SCHK]
 KCALL 5                       ; MemLock -> AX
 mov ds,ax
 mov ax,dx
 xor cx,cx
 KCALL 28                      ; LMemReAlloc(Chunk, 0), DS = Block
 DSDG
 mov bx,[V_SBLK]
 KCALL 6                       ; MemUnlock
 DSDG
 mov bx,[V_SBLK]
 mov ax,[V_SCHK]
 mov si,ax
 mov cl,0
 KCALL 281                     ; GrCreateGString -> DI
 DSDG
 push di
 mov si,[V_SRC]
 mov cx,0x2000
 KCALL 204                     ; GrCopyGString: ein Element
 DSDG
 pop di
 mov si,[V_SRC]
 mov dl,1
 KCALL 279                     ; GrDestroyGString, Daten behalten
 DSDG
 mov bx,[V_SBLK]
 KCALL 5
 DSDG
 mov [V_SSEG],ax
 mov es,ax
 mov si,[V_SCHK]
 mov si,[es:si]
 mov ax,[es:si-2]
 sub ax,2
 mov [V_ELEN],ax
 ret

unlock_scratch:
 mov bx,[V_SBLK]
 KCALL 6
 DSDG
 ret

; Attribute des aktuellen Elements aus dem Hilfs-GState lesen
query_attrs:
 mov di,[V_G]
 KCALL 259                     ; GrGetLineColor -> AL, BL, BH
 DSDG
 mov [V_LC],al
 mov [V_LC+1],bl
 mov [V_LC+2],bh
 mov di,[V_G]
 KCALL 260                     ; GrGetAreaColor
 DSDG
 mov [V_AC],al
 mov [V_AC+1],bl
 mov [V_AC+2],bh
 mov di,[V_G]
 KCALL 261                     ; GrGetTextColor
 DSDG
 mov [V_TC],al
 mov [V_TC+1],bl
 mov [V_TC+2],bh
 mov byte [V_INV],0
 mov di,[V_G]                  ; Muster (Draw-Masken) als Aufhellung
 xor ax,ax
 KCALL 262                     ; GrGetLineMask (AL=0) -> AL
 DSDG
 mov bx,V_LC
 call blend_mask
 jnc .v1
 or byte [V_INV],1
.v1:
 mov di,[V_G]
 xor ax,ax
 KCALL 263                     ; GrGetAreaMask
 DSDG
 mov byte [V_APAT],0
 cmp al,1                      ; Muster 1..24: als PDF-Kachelmuster
 jb .ag
 cmp al,24
 ja .ag
 mov [V_APAT],al
 jmp .v2
.ag:
 mov bx,V_AC
 call blend_mask
 jnc .v2
 or byte [V_INV],2
.v2:
 mov di,[V_G]
 xor ax,ax
 KCALL 264                     ; GrGetTextMask
 DSDG
 mov bx,V_TC
 call blend_mask
 jnc .v4
 or byte [V_INV],4
.v4:
 mov di,[V_G]
 xor ax,ax
 KCALL 271                     ; GrGetLineWidth -> AX
 DSDG
 mov [V_LW],ax
 mov di,[V_G]
 KCALL 293                     ; GrGetFont -> CX id, DX Groesse
 DSDG
 mov [V_FSIZE],dx
 mov [V_FID],cx
 mov di,[V_G]
 xor ax,ax
 KCALL 269                     ; GrGetTextStyle -> AL
 DSDG
 xor ah,ah
 mov [V_STYLE],ax
 mov di,[V_G]
 xor ax,ax
 xor bx,bx
 KCALL 275                     ; GrGetCurPos -> AX, BX
 DSDG
 mov [V_CPX],ax
 mov [V_CPY],bx
 mov di,[V_G]
 mov si,V_OM
 KCALL 286                     ; GrGetTransform -> DS:SI (24 Byte)
 DSDG
 cmp word [V_NEST],0
 je .nn
 call compose_outer            ; eingebettete Grafik: aeussere Matrix anwenden
.nn:
 ret

%include "emit.inc"
%include "fonts.inc"
%include "pattern.inc"
%include "embed.inc"

; ============================================================ Ausgabe
out_byte:                       ; AL
 push bx
 mov bx,[V_BUFLEN]
 mov [V_BUF+bx],al
 inc bx
 mov [V_BUFLEN],bx
 add word [V_POS],1
 adc word [V_POS+2],0
 cmp bx,BUFSZ
 jb .r
 call out_flush
.r:
 pop bx
 ret

out_flush:
 push ax
 push bx
 push cx
 push dx
 mov cx,[V_BUFLEN]
 jcxz .x
 cmp word [V_NOFILE],0
 jne .x
 mov bx,[V_FH]
 mov dx,V_BUF
 xor al,al
 KCALL 59                      ; FileWrite
 DSDG
.x:
 mov word [V_BUFLEN],0
 pop dx
 pop cx
 pop bx
 pop ax
 ret

out_inl:                        ; Inline-Text, alle Register bleiben erhalten
 push bp
 mov bp,sp
 push si
 push ax
 mov si,[bp+2]
.l:
 mov al,[cs:si]
 inc si
 or al,al
 jz .d
 call out_byte
 jmp .l
.d:
 mov [bp+2],si
 pop ax
 pop si
 pop bp
 ret

out_int:                        ; AX vorzeichenbehaftet, dezimal
 push ax
 push bx
 push cx
 push dx
 or ax,ax
 jns .p
 push ax
 mov al,'-'
 call out_byte
 pop ax
 neg ax
.p:
 xor cx,cx
 mov bx,10
.d:
 xor dx,dx
 div bx
 push dx
 inc cx
 or ax,ax
 jnz .d
.o:
 pop ax
 add al,'0'
 call out_byte
 loop .o
 pop dx
 pop cx
 pop bx
 pop ax
 ret

out_u32:                        ; DX:AX dezimal; SI<>0: 10 Stellen mit Nullen
 push ax
 push bx
 push cx
 push dx
 push di
 mov cx,10
 mov di,10
.d:
 mov bx,ax
 mov ax,dx
 xor dx,dx
 div di
 xchg ax,bx
 div di
 push dx
 mov dx,bx
 loop .d
 mov cx,10
 xor bx,bx                      ; BX=1 sobald eine Ziffer ausgegeben
.o:
 pop ax
 cmp cx,1
 je .put
 or si,si
 jnz .put
 or bx,bx
 jnz .put
 or al,al
 jz .nx
.put:
 mov bx,1
 add al,'0'
 call out_byte
.nx:
 loop .o
 pop di
 pop dx
 pop cx
 pop bx
 pop ax
 ret

out_fix:                        ; DX.CX WWFixed vorzeichenbehaftet
 push ax
 push bx
 push cx
 push dx
 test dx,0x8000
 jz .p
 mov al,'-'
 call out_byte
 neg cx
 adc dx,0
 neg dx
.p:
 mov ax,dx
 call out_int
 mov al,'.'
 call out_byte
 mov ax,cx
 mov bx,10000
 mul bx                         ; DX = Bruch * 10000 / 65536
 mov ax,dx
 mov cx,4
 mov bx,10
.d:
 xor dx,dx
 div bx
 push dx
 loop .d
 mov cx,4
.o:
 pop ax
 add al,'0'
 call out_byte
 loop .o
 pop dx
 pop cx
 pop bx
 pop ax
 ret

out_col:                        ; AL 0..255 -> 0.xxx
 push ax
 push bx
 push cx
 push dx
 cmp al,255
 jne .f
 mov al,'1'
 call out_byte
 jmp .x
.f:
 xor ah,ah
 mov bx,1000
 mul bx
 mov bx,255
 div bx
 push ax
 mov al,'0'
 call out_byte
 mov al,'.'
 call out_byte
 pop ax
 mov cx,3
 mov bx,10
.d:
 xor dx,dx
 div bx
 push dx
 loop .d
 mov cx,3
.o:
 pop ax
 add al,'0'
 call out_byte
 loop .o
.x:
 pop dx
 pop cx
 pop bx
 pop ax
 ret

out_rgb:                        ; [BX] = R, G, B
 push ax
 mov al,[bx]
 call out_col
 OUT " "
 mov al,[bx+1]
 call out_col
 OUT " "
 mov al,[bx+2]
 call out_col
 pop ax
 ret

out_hex2:
 push ax
 push cx
 mov ah,al
 mov cl,4
 shr al,cl
 call .n
 mov al,ah
 and al,15
 call .n
 pop cx
 pop ax
 ret
.n:
 add al,'0'
 cmp al,'9'
 jbe .o
 add al,7
.o:
 jmp out_byte

out_pdfchar:                    ; AL als Zeichen in einem PDF-String
 cmp al,'('
 je .esc
 cmp al,')'
 je .esc
 cmp al,'\'
 je .esc
 cmp al,32
 jb .oct
 cmp al,127
 jae .oct
 jmp out_byte
.esc:
 push ax
 mov al,'\'
 call out_byte
 pop ax
 jmp out_byte
.oct:
 push ax
 push cx
 mov ah,al
 mov al,'\'
 call out_byte
 mov al,ah
 mov cl,6
 shr al,cl
 add al,'0'
 call out_byte
 mov al,ah
 mov cl,3
 shr al,cl
 and al,7
 add al,'0'
 call out_byte
 mov al,ah
 and al,7
 add al,'0'
 call out_byte
 pop cx
 pop ax
 ret

font_names:
 db "Times-Roman",0,"Times-Bold",0,"Times-Italic",0,"Times-BoldItalic",0
 db "Helvetica",0,"Helvetica-Bold",0,"Helvetica-Oblique",0,"Helvetica-BoldOblique",0
 db "Courier",0,"Courier-Bold",0,"Courier-Oblique",0,"Courier-BoldOblique",0
 db "Symbol",0,"ZapfDingbats",0

code_end:
section .relocs
relocs_end:
section .footer follows=.relocs align=1
 db 'PDFMETA1'
 dw code_end
 dw relocs_end
