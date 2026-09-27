#!/bin/bash
set -xue

OBJCOPY=/opt/homebrew/opt/llvm/bin/llvm-objcopy

CC=/opt/homebrew/opt/llvm/bin/clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fuse-ld=lld -fno-stack-protector -ffreestanding -nostdlib"

# shell
$CC $CFLAGS -Wl,-Tuser.ld,-Map=shell.map -o shell.elf shell.c user.c io.c
# convert elf file into raw binary
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary shell.elf shell.bin
# convert raw binary into a format that can be embedded in C language
$OBJCOPY -Ibinary -Oelf32-littleriscv shell.bin shell.bin.o

# kernel
$CC $CFLAGS -Wl,-TKernel.ld -Wl,-Map=kernel.map -o kernel.elf kernel.c io.c sbi.c memory.c string.c process.c shell.bin.o

qemu-system-riscv32 -machine virt -bios default -nographic -serial mon:stdio --no-reboot -kernel kernel.elf