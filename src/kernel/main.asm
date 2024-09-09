org 0x0
bits 16


%define ENDL 0x0D, 0x0A


start:
    ; Print done message
    mov si, msg_done
    call puts

.halt:
    cli
    hlt


;
; Prints a string to the screen
; Args:
;   ds:si - Pointer to the string
;
puts:
    ; Save registers that will be modified
    push si
    push ax
    push bx

.loop:
    lodsb           ; Load next char in al
    or al, al       ; Check if char is null
    jz .done


    mov ah, 0x0E    ; BIOS teletype function
    xor bh, bh      ; Page number 0
    int 0x10        ; Call BIOS

    jmp .loop

.done:
    pop bx
    pop ax
    pop si
    ret

msg_done: db 'Kernel loaded!', ENDL, 0
