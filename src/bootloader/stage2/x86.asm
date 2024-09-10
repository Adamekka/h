bits 16

section _TEXT class=CODE

;
; void _cdecl x86_div64_32(uint64_t dividend, uint32_t divisor, uint64_t* quotient, uint32_t* remainder);
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


;
; Prints a char to the screen
; Args:
;   character
;   page
;
global _x86_Video_WriteCharTeletype
_x86_Video_WriteCharTeletype:
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
