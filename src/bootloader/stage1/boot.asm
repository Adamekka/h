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
bdb_sectors_per_cluster:    db 1
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
    ; Setup data segments
    xor ax, ax                  ; Can't write to ds/ex directly, must use intermediate register
    mov ds, ax
    mov es, ax

    ; Setup stack
    mov ss, ax
    mov sp, 0x7C00              ; Stack grows down from 0x7C00

    ; Some BIOSes might start us at 07C0:0000, so we need to jump to 0000:7C00
    push es
    push word .after
    retf

.after:
    ; Read from disk
    mov [ebr_drive_number], dl      ; BIOS should set dl to drive number

    ; Print booting message
    mov si, msg_booting
    call puts

    ; Read drive parameters (sectors per track, head count), instead of relying on data on disk
    push es
    mov ah, 08h
    int 13h
    jc floppy_error
    pop es

    and cl, 0x3F                    ; Lower 6 bits are sector count, remove top 2 bits
    xor ch, ch
    mov [bdb_sectors_per_track], cx ; Sector count

    inc dh
    mov [bdb_heads], dh             ; Head count

    ; Calculate LBA of root directory = reserved + fats * sectors_per_fat
    mov ax, [bdb_sectors_per_fat]
    mov bl, [bdb_fat_count]
    xor bh, bh
    mul bx                          ; ax *= bx
    add ax, [bdb_reserved_sectors]  ; ax += reserved sectors
    push ax                         ; Save LBA of root directory

    ; Calculate size of root directory = (32 * dir_entries_count) / bytes_per_sector
    mov ax, [bdb_dir_entries_count]
    shl ax, 5                       ; ax *= 32
    xor dx, dx
    div word [bdb_bytes_per_sector] ; ax /= bytes_per_sector

    test dx, dx                     ; if dx != 0, add 1
    jz .root_dir_after
    inc ax                          ; Division remainder !=0, add 1
                                    ; This is to ensure we read the entire root directory

.root_dir_after:
    ; Read root directory
    mov cl, al                      ; cl = number of sectors to read
    pop ax                          ; ax = LBA of root directory
    mov dl, [ebr_drive_number]      ; dl = drive number
    mov bx, buffer                  ; es:bx = buffer
    call disk_read


    ; Search for the kernel.bin file
    xor bx, bx
    mov di, buffer

.search_kernel:
    mov si, file_stage2_bin
    mov cx, 11                      ; Compare up to 11 chars
    push di
    repe cmpsb
    pop di
    je .found_kernel

    add di, 32
    inc bx
    cmp bx, [bdb_dir_entries_count]
    jl .search_kernel

    ; Kernel not found
    jmp kernel_not_found_error

.found_kernel:
    ; di should point to the first byte of the file entry
    mov ax, [di + 26]             ; ax = First logical cluster field (offset 26)
    mov [stage2_cluster], ax

    ; Load FAT from disk to memeory
    mov ax, [bdb_reserved_sectors]  ; LBA of first FAT
    mov bx, buffer                  ; es:bx = buffer
    mov cl, [bdb_sectors_per_fat]   ; Number of sectors to read
    mov dl, [ebr_drive_number]      ; Drive number
    call disk_read

    ; Load kernel.bin to memory and process FAT chain
    mov bx, KERNEL_LOAD_SEGMENT
    mov es, bx
    mov bx, KERNEL_LOAD_OFFSET

.load_kernel_loop:
    ; Read next cluster
    mov ax, [stage2_cluster]
    add ax, 31                      ; TODO: Hardcoded 31, should be bytes per cluster - 1
                                    ; first cluster = (stage2_cluster - 2) * sectors_per_cluster + data_start
                                    ; start sector = reserved + fats + root dir size = 1 + 18 + 134 = 33
    mov cl, 1
    mov dl, [ebr_drive_number]
    call disk_read

    add bx, [bdb_bytes_per_sector]  ; TODO: Add will overflow if kernel.bit is larger than 64KB, need to check for overflow

    ; Compute location of next cluster
    mov ax, [stage2_cluster]
    mov cx, 3
    mul cx
    mov cx, 2
    div cx                          ; ax = index entry in FAT, dx = cluster % 2

    mov si, buffer
    add si, ax
    mov ax, [ds:si]                 ; Read entry from FAT table at index ax

    or dx, dx
    jz .even

.odd:
    shr ax, 4
    jmp .next_cluster_after

.even:
    and ax, 0x0FFF
    jmp .next_cluster_after

.next_cluster_after:
    cmp ax, 0x0FF8                  ; End of file
    jae .read_finished

    mov [stage2_cluster], ax
    jmp .load_kernel_loop

.read_finished:
    ; Jump to kernel
    mov dl, [ebr_drive_number]      ; Boot device in dl

    mov ax, KERNEL_LOAD_SEGMENT     ; Set segment registers
    mov ds, ax
    mov es, ax

    jmp KERNEL_LOAD_SEGMENT:KERNEL_LOAD_OFFSET

    ; Unreachable code
    jmp wait_for_key_and_reboot

    cli                             ; Disable interrupts
    hlt


;
; Error handlers
;

floppy_error:
    mov si, msg_disk_read_failed
    call puts
    jmp wait_for_key_and_reboot

kernel_not_found_error:
    mov si, msg_stage2_not_found
    call puts
    jmp wait_for_key_and_reboot

wait_for_key_and_reboot:
    xor ah, ah
    int 16h         ; Wait for key press
    jmp 0FFFFh:0    ; Reboot

.halt:
    cli             ; Disable interrupts, so we don't get stuck in an infinite loop
    hlt


;
; Prints a string to the screen
; Args:
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
    xor bh, bh      ; Page number 0
    int 0x10        ; Call BIOS

    jmp .loop

.done:
    pop bx
    pop ax
    pop si
    ret


;
; Disk routines
;

;
; Converts an LBA address to a CHS address
; Args:
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

    pop ax
    mov dl, al                          ; dl = head
    pop ax
    ret


;
; Reads sectors from the disk
; Args:
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
; Args:
;   dl - Drive number
;
disk_reset:
    pusha
    xor ah, ah
    stc
    int 13h
    jc floppy_error
    popa
    ret


msg_booting:            db 'Booting...', ENDL, 0
msg_disk_read_failed:   db 'Read from disk failed!', ENDL, 0
msg_stage2_not_found:   db 'STAGE2.BIN file not found!', ENDL, 0
file_stage2_bin:        db 'STAGE2  BIN'
stage2_cluster:         dw 0

KERNEL_LOAD_SEGMENT     equ 0x2000
KERNEL_LOAD_OFFSET      equ 0


times 510-($-$$) db 0
dw 0AA55h


buffer:
