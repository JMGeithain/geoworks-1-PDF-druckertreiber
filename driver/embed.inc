; ============================================================ Eingebettete Grafiken
; GeoWrite legt Grafiken im Text als Zeichen 1Ah ab. Im Druck-GString steht dann
; hinter dem festen Teil des Textfeldes (3 + fcount) ein vollstaendiger 1.x-GString
; mit der Grafik. Er wird in einen eigenen Block kopiert, geladen und mit einem
; eigenen Hilfs-GState abgespielt. Die Lage: Ursprung (Feld-x, Zeilenoberkante);
; die Matrix des Feldes wird jeder inneren Matrix nachgeschaltet (V_OUTER).

; ES:SI = Textfeld, BX = Abstand des GStrings im Element.
; CF = 1, wenn keine eingebettete Grafik vorhanden ist.
tf_embedded:
 cmp word [V_NEST],0            ; nur eine Ebene
 jne .no
 mov cx,bx                      ; Beginn des GStrings (ab Elementanfang)
 cmp byte [es:si+bx],4          ; beginnt mit GR_SAVE_STATE
 jne .no
 mov ax,[V_ELEN]                ; Elementlaenge (mit Opcode)
 cmp ax,cx
 jbe .no
 sub ax,cx
 cmp ax,2
 jb .no
 mov [V_TFN],ax                 ; Laenge des GStrings
 mov [V_TFC],cx                 ; Abstand im Element
 ; Lage: y = Zeilenoberkante (Grundlinie - baseLine bei TM_DRAW_BASE)
 mov bx,[es:si+TF_SAVED+24]
 mov di,cx
 add di,si
 sub di,22                      ; letzter Textabschnitt vor dem GString
 test byte [es:di+2+8],0x08
 jz .yt
 sub bx,[es:si+TF_SAVED+26]
.yt:
 mov ax,[V_TFX]
 ; aeussere Matrix: Feldmatrix mit Ursprung (AX, BX)
 mov word [V_PX],0
 mov [V_PX+2],ax
 mov word [V_PY],0
 mov [V_PY+2],bx
 call xform
 push si
 push es
 mov si,V_OM
 mov di,V_OUTER
 mov cx,8
.cp:
 mov ax,[si]
 mov [di],ax
 add si,2
 add di,2
 loop .cp
 pop es
 pop si
 mov ax,[V_QX]
 mov [V_OUTER+16],ax
 mov ax,[V_QX+2]
 mov [V_OUTER+18],ax
 mov ax,[V_QY]
 mov [V_OUTER+20],ax
 mov ax,[V_QY+2]
 mov [V_OUTER+22],ax
 ; GString in eigenen LMem-Block kopieren
 push es
 push si
 push word [V_TFN]
 mov ax,[V_TFN]
 add ax,0x40
 mov cx,0x6050
 KCALL 0                        ; MemAlloc (gesperrt) -> BX Handle, AX Segment
 push bx
 mov ds,ax
 xor ax,ax
 mov cx,0x060a
 mov dx,0x10
 KCALL 24                       ; LMemInitHeap (DS = Block)
 push bp
 mov bp,sp
 mov cx,[ss:bp+4]               ; Laenge
 pop bp
 KCALL 25                       ; LMemAlloc -> AX Chunk (DS ggf. neu)
 mov dx,ax                      ; DX = Chunk
 mov di,ax
 mov di,[di]                    ; DS:DI = Ziel
 mov ax,ds
 pop bx                         ; Handle
 pop cx                         ; Laenge
 pop si
 pop es                         ; ES:SI = Element
 push es
 push si
 DSDG
 mov [V_EBLK],bx
 add si,[V_TFC]
 push ds
 push es
 pop ds                         ; DS:SI = Quelle
 mov es,ax                      ; ES:DI = Ziel
 cld
 rep movsb
 pop ds
 mov bx,[V_EBLK]
 KCALL 6                        ; MemUnlock
 DSDG
 ; Aeusseren Zustand sichern, Kopierpuffer freigeben (wird innen gebraucht)
 call unlock_scratch
 push word [V_SRC]
 push word [V_G]
 push word [V_RET]
 push word [V_ITER]
 mov bx,[V_EBLK]
 mov si,dx
 mov cl,0
 KCALL 280                      ; GrLoadGString(Chunk) -> SI
 DSDG
 mov [V_SRC],si
 xor di,di
 KCALL 211                      ; GrCreateState -> DI
 DSDG
 mov [V_G],di
 mov word [V_NEST],1
 mov word [V_ITER],0
 OUT "q",10
 call play_loop
 OUT "Q",10
 mov word [V_NEST],0
 mov di,[V_G]
 KCALL 212                      ; GrDestroyState
 DSDG
 mov di,[V_SRC]                 ; GString-Handle in DI (wie SPOOL.GEO)
 mov dl,1
 KCALL 279                      ; GrDestroyGString, Daten behalten
 DSDG
 mov bx,[V_EBLK]
 KCALL 2                        ; MemFree
 DSDG
 pop word [V_ITER]
 pop word [V_RET]
 pop word [V_G]
 pop word [V_SRC]
 ; Kopierpuffer wieder sperren (der Aufrufer gibt ihn frei)
 mov bx,[V_SBLK]
 KCALL 5
 DSDG
 pop si
 pop es
 mov es,ax
 mov si,[V_SCHK]
 mov si,[es:si]
 clc
 ret
.no:
 stc
 ret

; V_OM := V_OM * V_OUTER (erst innere, dann aeussere Abbildung)
%macro MM 5
 mov cx,[V_TA+%2]
 mov dx,[V_TA+%2+2]
 mov bx,V_OUTER+%3
 call mulfix
 push dx
 push cx
 mov cx,[V_TA+%4]
 mov dx,[V_TA+%4+2]
 mov bx,V_OUTER+%5
 call mulfix
 pop ax
 add cx,ax
 pop ax
 adc dx,ax
 mov [V_OM+%1],cx
 mov [V_OM+%1+2],dx
%endmacro
compose_outer:
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 mov si,V_OM
 mov di,V_TA
 mov cx,12
.c:
 mov ax,[si]
 mov [di],ax
 add si,2
 add di,2
 loop .c
 MM 0, 0,0, 4,8
 MM 4, 0,4, 4,12
 MM 8, 8,0, 12,8
 MM 12, 8,4, 12,12
 MM 16, 16,0, 20,8
 mov ax,[V_OUTER+16]
 add [V_OM+16],ax
 mov ax,[V_OUTER+18]
 adc [V_OM+18],ax
 MM 20, 16,4, 20,12
 mov ax,[V_OUTER+20]
 add [V_OM+20],ax
 mov ax,[V_OUTER+22]
 adc [V_OM+22],ax
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 ret
