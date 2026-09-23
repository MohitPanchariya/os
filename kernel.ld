/* boot function is the entry point of the program */
ENTRY(boot)

/* '.' represents current address (virtual memory address) */

SECTIONS {
    /* base address */
    . = 0x80200000;
    __kernel_base = .;

    .text :{
        /* .text.boot is at the top in text section */
        KEEP(*(.text.boot));
        *(.text .text.*);
    }

    .rodata : ALIGN(4){
        *(.rodata .rodata.*);
    }

    .data : ALIGN(4) {
        *(.data .data.*);
    }

    .bss : ALIGN(4) {
        /*
        set __bss to current address
        __bss is accessed in the kernel
        */
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*);
        __bss_end = .;
    }

    . = ALIGN(4);
    . += 128 * 1024; /* 128KB */
    /* create a 128KB stack */
    __stack_top = .;

    . = ALIGN(4096);
    __free_ram = .;
    . += 64 * 1024 * 1024; /* 64MB RAM */
    __free_ram_end = .;
}