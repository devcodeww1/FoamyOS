CC = gcc
AS = as
LD = ld

CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -std=gnu99 -fno-pie -fno-stack-protector
ASFLAGS = --32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

OBJS = boot.o interrupts.o kernel.o lib.o vga.o serial.o gdt.o idt.o keyboard.o shell.o

all: foamyos.bin foamyos.iso check

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

boot.o: boot.s
	$(CC) $(CFLAGS) -c $< -o $@

interrupts.o: interrupts.s
	$(CC) $(CFLAGS) -c $< -o $@

foamyos.bin: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

foamyos.iso: foamyos.bin grub.cfg
	mkdir -p iso/boot/grub
	cp foamyos.bin iso/boot/foamyos.bin
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o foamyos.iso iso

check: foamyos.bin
	grub-file --is-x86-multiboot foamyos.bin && echo "Multiboot check PASSED!"

run: foamyos.iso
	qemu-system-x86_64 -m 512M -cdrom foamyos.iso

clean:
	rm -rf *.o foamyos.bin foamyos.iso iso

.PHONY: all check run clean
