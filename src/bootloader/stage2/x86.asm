bits 16

section _TEXT class=CODE


; MARK: Helpers


;
; U4M
;
; Operation:    Integer 4 byte multiply
; Inputs:       DX;AX   Multiplicand
;               CX;BX   Multiplier
; Outputs:      DX;AX   Product
; Volatile:     CX, BX  destroyed
;
global __U4M
__U4M:
    shl edx, 16     ; dx to upper half of edx
    mov dx, ax      ; edx - multiplicand
    mov eax, edx    ; eax - multiplicand

    shl ecx, 16     ; cx to upper half of ecx
    mov cx, bx      ; cx - multiplier

    mul ecx         ; edx:eax - product
    mov edx, eax    ; Move upper 32 bits to edx
    shr edx, 16

    ret


;
; U4D
;
; Operation:    Unsigned 4 byte divide
; Inputs:       DX;AX   Dividend
;               CX;BX   Divisor
; Outputs:      DX;AX   Quotient
;               CX;BX   Remainder
; Volatile:     none
;
global __U4D
__U4D:
    shl edx, 16     ; dx to upper half of edx
    mov dx, ax      ; edx - dividend
    mov eax, edx    ; eax - dividend
    xor edx, edx

    shl ecx, 16     ; cx to upper half of ecx
    mov cx, bx      ; cx - divisor

    div ecx         ; eax - quotient, edx - remainder
    mov ebx, edx
    mov ecx, edx
    shr ecx, 16

    mov edx, eax
    shr edx, 16

    ret


;
; void _cdecl x86_div64_32(
;     uint64_t dividend, uint32_t divisor, uint64_t* quotient, uint32_t* remainder
; );
;
global _x86_div64_32
_x86_div64_32:
    ; Make new call frame
    push bp             ; Save old base pointer
    mov bp, sp          ; Set new base pointer

    push bx

    ; Divide upper 32 bits of dividend by divisor
    mov eax, [bp + 8]   ; Load upper 32 bits of dividend
    mov ecx, [bp + 12]  ; Load divisor
    xor edx, edx        ; Clear edx
    div ecx             ; Divide

    ; Store upper 32 bits of quotient
    mov bx, [bp + 16]   ; Load quotient pointer
    mov [bx + 4], eax   ; Store upper 32 bits of quotient

    ; Divide lower 32 bits of dividend by divisor
    mov eax, [bp + 4]   ; Load lower 32 bits of dividend
    div ecx             ; Divide

    ; Store results
    mov [bx], eax       ; Store quotient
    mov bx, [bp + 18]   ; Load remainder pointer
    mov [bx], edx       ; Store remainder

    pop bx

    ; Restore old call frame
    mov sp, bp          ; Restore stack pointer
    pop bp              ; Restore old base pointer
    ret


; MARK: Video

;
; void _cdecl x86_Video_write_char_teletype(char c, uint8_t page);
;
global _x86_Video_write_char_teletype
_x86_Video_write_char_teletype:
    ; Make new call frame
    push bp             ; Save old base pointer
    mov bp, sp          ; Set new base pointer

    push bx             ; Save bx

                        ; [bp + 0] Old call frame
                        ; [bp + 2] Return address
                        ; [bp + 4] Character (first argument)
                        ; bytes are converted to words (you can't push a single byte on the stack)
                        ; [bp + 6] Page (second argument)

    mov ah, 0x0E
    mov al, [bp + 4]    ; Load character
    mov bh, [bp + 6]    ; Load page number

    int 0x10            ; Call BIOS

    pop bx              ; Restore bx

    ; Restore old call frame
    mov sp, bp          ; Restore stack pointer
    pop bp              ; Restore old base pointer
    ret


; MARK: Disk

;
; bool _cdecl x86_Disk_reset(uint8_t drive);
;
global _x86_Disk_reset
_x86_Disk_reset:
    ; Make new call frame
    push bp             ; Save old base pointer
    mov bp, sp          ; Set new base pointer

    xor ah, ah
    mov dl, [bp + 4]    ; Load drive number
    stc
    int 0x13

    mov ax, 1
    sbb ax, 0           ; Clear carry flag if error

    ; Restore old call frame
    mov sp, bp          ; Restore stack pointer
    pop bp              ; Restore old base pointer
    ret


;
; bool _cdecl x86_Disk_read(
;     uint8_t drive_number,
;     uint16_t cylinder,
;     uint16_t sector,
;     uint16_t head,
;     uint8_t sector_count,
;     uint8_t far* buffer
; );
;
global _x86_Disk_read
_x86_Disk_read:
    ; Make new call frame
    push bp             ; Save old base pointer
    mov bp, sp          ; Set new base pointer

    ; Save registers
    push bx
    push es

    ; Args
    mov dl, [bp + 4]    ; dl - drive number

    mov ch, [bp + 6]    ; ch - cylinder (lower 8 bits)
    mov cl, [bp + 7]    ; cl - cylinder (bits 6-7)
    shl cl, 6

    mov al, [bp + 8]    ; cl - sector (bits 0-5)
    and al, 0x3F
    or cl, al

    mov dh, [bp + 10]   ; dh - head

    mov al, [bp + 12]   ; al - number of sectors to read

    mov bx, [bp + 16]   ; es:bx - far pointer to buffer
    mov es, bx
    mov bx, [bp + 14]

    ; Interrupt
    mov ah, 0x02
    stc
    int 0x13

    ; Return value
    mov ax, 1
    sbb ax, 0           ; Clear carry flag if error

    ; Restore registers
    pop es
    pop bx

    ; Restore old call frame
    mov sp, bp          ; Restore stack pointer
    pop bp              ; Restore old base pointer
    ret


;
; bool _cdecl x86_Disk_get_drive_parameters(
;     uint8_t drive,
;     uint8_t* drive_type,
;     uint16_t* cylinders,
;     uint16_t* sectors,
;     uint16_t* heads
; );
;
global _x86_Disk_get_drive_parameters
_x86_Disk_get_drive_parameters:
    ; Make new call frame
    push bp             ; Save old base pointer
    mov bp, sp          ; Set new base pointer

    ; Save registers
    push es
    push bx
    push si
    push di

    ; Interrupt
    mov dl, [bp + 4]    ; dl - drive number
    mov ah, 0x08
    xor di, di          ; es:di = 0000:0000
    mov es, di
    stc
    int 0x13

    ; Return value
    mov ax, 1
    sbb ax, 0           ; Clear carry flag if error

    ; Return results
    mov si, [bp + 6]    ; si - type
    mov [si], bl

    mov bl, ch          ; cylinders - lower bits in ch
    mov bh, cl          ; cylinders - upper bits in cl (6-7)
    shr bh, 6
    mov si, [bp + 8]    ; si - cylinders*
    mov [si], bx

    xor ch, ch          ; sectors - lower bits in cH
    and cl, 0x3F
    mov si, [bp + 10]
    mov [si], cx

    mov cl, dh          ; heads - dh
    mov si, [bp + 12]
    mov [si], cx

    ; Restore registers
    pop di
    pop si
    pop bx
    pop es

    ; Restore old call frame
    mov sp, bp          ; Restore stack pointer
    pop bp              ; Restore old base pointer
    ret
