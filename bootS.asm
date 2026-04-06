[BITS 16]
org 0x7C00

cli 
xor ax, ax ; setup segment registers
mov ds, ax
mov es, ax

mov ax, 0x7000 ; setup a 3kb stack
mov ss, ax
mov sp, 0x7C00
sti

xor ax, ax ; reset ax for safety

mov ah, 0x02 ; BIOS read sector function 
mov al, 62 ; number of sectors to read = 62 (31kb)
mov ch, 0 ; cylinder 0
mov cl, 2 ; starting sector = 2.. right after the bootloader
mov dh, 0 ; head 0
mov dl, 0x80 ; drive 0x80 = first hard disk
mov bx, stage2_entry ; load the second stage at stage2_entry (0x7E00)
int 0x13 ; call BIOS disk services
jc disk_error ; if carry flag is set, there was an error



jmp stage2_entry ; jump to the second stage of the bootloader




disk_error:
    mov al, ah ; save error code in al for hex conversion
    shr al, 4 ; get high nibble
    call hex_to_nibble ; convert high nibble to ASCII
    mov [high_nibble], al ; store high nibble ASCII
    mov al, ah ; get error code again
    and al, 0x0F ; get low nibble
    call hex_to_nibble ; convert low nibble to ASCII
    mov [low_nibble], al ; store low nibble ASCII
    call print_error_code ; print "Disk error: 0xXY" where XY is the error code in hex
    halt:
    jmp halt ; infinite loop to halt the system

hex_to_nibble:
    cmp al, 9
    jbe digit

    add al, 7 ; convert to uppercase letter

    digit:
    add al, '0' ; convert to ASCII
    ret

print_error_code:
    mov bh, 0 ; page number
    mov ah, 0x0E ; BIOS teletype output function
    mov al, '0'
    int 0x10
    mov al, 'x'
    int 0x10
    mov al, [high_nibble]
    int 0x10
    mov al, [low_nibble]
    int 0x10
    ret

high_nibble db 0
low_nibble db 0

times 510-($-$$) db 0 ; pad the rest of the boot sector with zeros
dw 0xAA55 ; boot signature

stage2_entry:

mov ax, 0x0003
int 0x10

jmp switch_to_protected_mode

gdt_start:
    dd 0x0
    dd 0x0


    dw 0xffff ; code segment
    dw 0x0000 
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

    dw 0xffff ; data segment
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

switch_to_protected_mode:
cli
lgdt [gdt_descriptor]
mov eax, cr0
or eax, 1
mov cr0, eax
jmp 0x08:protected_mode_entry

[BITS 32]
protected_mode_entry:
	mov ax, 0x10
	mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    mov esp, 0x7c00
    
	jmp next
    
  
	
	
	sector_count dw 0
	lba_address dd 0
	buffer_address dd 0
	
	read_kernel:
	mov dword [lba_address], 64
	mov dword [buffer_address], 0x00100000
	mov ecx, 2048
	read_kernel_loop:
	call read_sectors
	call load_to_ram
	inc dword [lba_address]
	add dword [buffer_address], 512
	loop read_kernel_loop
	ret
	
	read_sectors:
	push eax
	push edx
	mov dx, 0x1f2
	mov al, 1
	out dx, al
	inc dx
	mov eax, [lba_address]
	out dx, al
	inc dx
	shr eax, 8
	out dx, al
	inc dx
	shr eax, 8
	out dx, al
	inc dx
	shr eax, 8
	and al, 0x0f
	or al, 0xe0
	out dx, al
	inc dx
	mov al, 0x20
	out dx, al
	wait_bsy:
	in al, dx
	test al, 0x80
	jnz wait_bsy
	wait_drq:
	in al, dx
	test al, 0x08
	jz wait_drq
	pop edx
	pop eax
	ret
	
	load_to_ram:
	push edx
	push edi
	push ecx
	mov dx, 0x1f0
	mov edi, [buffer_address]
	mov ecx, 256
	load_loop:
	in ax, dx
	mov [edi], ax
	add edi, 2
	loop load_loop
	pop ecx
	pop edi
	pop edx
	ret
	
	
	next:
	
	call read_kernel
	mov esp, 0x00090000
	jmp 0x00100000
	
	
	

times 32768-($-$$) db 0 

