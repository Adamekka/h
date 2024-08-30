bits 16

section _TEXT class=CODE

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

    mov ah, 0Eh
    mov al, [bp + 4]    ; Load character
    mov bh, [bp + 6]    ; Load page number

    int 10h             ; Call BIOS

    pop bx              ; Restore bx

    ; Restore old call frame
    mov sp, bp          ; Restore stack pointer
    pop bp              ; Restore old base pointer
    ret
