nasm -f bin bootS.asm -o bootS.bin

dd if=bootS.bin of=disk.img bs=512 conv=notrunc

gcc -m32 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -nostdlib -nostartfiles -c kernel.c -o kernel.o
ld -m elf_i386 -T link.ld -o kernel.elf kernel.o
objcopy -O binary kernel.elf kernel.bin
truncate -s 1048576 kernel.bin


dd if=kernel.bin of=disk.img bs=512 seek=64 conv=notrunc


qemu-system-i386 -m 512 -drive file=disk.img,format=raw
