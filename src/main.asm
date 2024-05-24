org 0x7C00
bits 16


%define ENDL 0x0D, 0x0A


start:
    jmp main


;
; Prints a string to the screen
; Params:
;   si - Pointer to the string
;
puts:
    ; Save registers that will be modified
    push si
    push ax
    push bh

.loop:
    lodsb          ; Load next char in al
    or al, al      ; Check if char is null
    jz .done

    mov ah, 0x0E   ; BIOS teletype function
    mov bh, 0      ; Page number 0
    int 0x10       ; Call BIOS

    jmp .loop

.done:
    pop bx
    pop ax
    pop si
    ret


main:
    ; Setup data segments
    mov ax, 0      ; Can't write to ds/ex directly, must use intermediate register
    mov ds, ax
    mov es, ax

    ; Setup stack
    mov ss, ax
    mov sp, 0x7C00 ; Stack grows down from 0x7C00

    ; Print done message
    mov si, msg_done
    call puts

    hlt

.halt:
    jmp .halt


msg_done: db 'Done!', ENDL, 0


times 510-($-$$) db 0
dw 0AA55h
