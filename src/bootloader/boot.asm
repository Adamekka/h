org 0x7C00
bits 16


%define ENDL 0x0D, 0x0A

;
; FAT12 header
;
jmp short start
nop

bdb_oem:                    db 'MSWIN4.1'           ; 8 bytes
bdb_bytes_per_sector:       dw 512
bdb_sectors_per_cluseter:   db 1
bdb_reserved_sectors:       dw 1
bdb_fat_count:              db 2
bdb_dir_entries_count:      dw 0E0h
bdb_total_sectors:          dw 2880                 ; 2880 * 512 = 1.44MB
bdb_media_descriptor_type:  db 0F0h                 ; F0 = 3.5" 1.44MB floppy disk
bdb_sectors_per_fat:        dw 9                    ; 9 sectors per track
bdb_sectors_per_track:      dw 18
bdb_heads:                  dw 2
bdb_hidden_sectors:         dd 0
bdb_large_sector_count:     dd 0

; Extended boot record
ebr_drive_number:           db 0                    ; 0x00 = floppy, 0x80 = hdd
                            db 0                    ; Reserved
ebr_signature:              db 0x29                 ; 0x29 = FAT12/16, 0x28 = FAT32
ebr_volume_id:              db 12h, 34h, 56h, 67h   ; Serial number, random number
ebr_volume_label:           db 'hOS         '       ; 11 bytes, padded with spaces
ebr_system_id:              db 'FAT12   '           ; 8 bytes, padded with spaces

;
; Entrypoint
;

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
    push bx

.loop:
    lodsb           ; Load next char in al
    or al, al       ; Check if char is null
    jz .done

    mov ah, 0x0E    ; BIOS teletype function
    mov bh, 0       ; Page number 0
    int 0x10        ; Call BIOS

    jmp .loop

.done:
    pop bx
    pop ax
    pop si
    ret


main:
    ; Setup data segments
    mov ax, 0                   ; Can't write to ds/ex directly, must use intermediate register
    mov ds, ax
    mov es, ax

    ; Setup stack
    mov ss, ax
    mov sp, 0x7C00              ; Stack grows down from 0x7C00

    ; Read from disk
    mov [ebr_drive_number], dl  ; BIOS should set dl to drive number
    mov ax, 1                   ; LBA = 1, first sector after boot sector
    mov cl, 1                   ; Read 1 sector
    mov bx, 0x7E00              ; Buffer address
    call disk_read

    ; Print done message
    mov si, msg_done
    call puts

    cli                         ; Disable interrupts
    hlt


;
; Error handlers
;

floppy_error:
    mov si, msg_disk_read_failed
    call puts
    jmp wait_for_key_and_reboot

wait_for_key_and_reboot:
    mov ah, 0
    int 0x16    ; Wait for key press
    int 0x19    ; Reboot

.halt:
    cli         ; Disable interrupts, so we don't get stuck in an infinite loop
    hlt


;
; Disk routines
;

;
; Converts an LBA address to a CHS address
; Params:
;   ax              - LBA address
; Returns:
;   cx [bits 0-5]   - Sector
;   cx [bits 6-15]  - Cylinder
;   dx - head
;
lba_to_chs:
    push ax
    push dx

    xor dx, dx  ; dx = 0
    div word [bdb_sectors_per_track]    ; ax = LBA / sectors_per_track
                                        ; dx = LBA % sectors_per_track
    inc dx                              ; dx = LBA % sectors_per_track + 1 = sector
    mov cx, dx                          ; cx = sector

    xor dx, dx                          ; dx = 0
    div word [bdb_heads]                ; ax = LBA / scetors_per_track / heads = cylinder
                                        ; dx = LBA / sectors_per_track % heads = head

    mov dh, dl                          ; dh = head
    mov ch, al                          ; ch = cylinder (lower 8 bits)
    shl ah, 6                           ; Shift upper 2 bits to the left
    or cl, ah                           ; ah = cylinder (upper 2 bits)

    pop dx
    mov dl, al                          ; dl = head
    pop ax
    ret


;
; Reads sectors from the disk
; Params:
;   ax      - LBA a
;   cl      - Number of sectors to read (up to 128)
;   dl      - Drive number
;   es:bx   - Buffer to read to
;
disk_read:
    ; Save registers that will be modified
    push ax         ; Save LBA address
    push bx         ; Save buffer address
    push cx         ; Save number of sectors
    push dx         ; Save drive number
    push di         ; Save retry counter

    push cx         ; Save number of sectors
    call lba_to_chs ; Convert LBA to CHS
    pop ax          ; AL = number of sectors to read

    mov ah, 02h
    mov di, 3       ; Number of retries

.retry:
    pusha           ; Save all registers
    stc             ; Set carry flag, some BIOSes require this
    int 13h         ; Carry flag is set if error
    jnc .done

    ; Read failed
    popa            ; Restore all register
    test di, di     ; Check if we have retries left
    jnz .retry

.fail:
    ; After all retries failed
    jmp floppy_error

.done:
    popa

    pop di          ; Restore retry counter
    pop dx          ; Restore drive number
    pop cx          ; Restore number of sectors
    pop bx          ; Restore buffer address
    pop ax          ; Restore LBA address
    ret


;
; Resets disk controller
; Params:
;   dl - Drive number
;
disk_reset:
    pusha
    mov ah, 0
    stc
    int 13h
    jc floppy_error
    popa
    ret


msg_done:               db 'Done!', ENDL, 0
msg_disk_read_failed:   db 'Read from disk failed!', ENDL, 0


times 510-($-$$) db 0
dw 0AA55h
