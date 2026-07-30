extern char __stack_top[], __bss[], __bss_end[];

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;


void* memset(void* buf, char fill, size_t buf_size) {
    uint8_t* p = (uint8_t* ) buf;
    while(buf_size--) {
        *p++ = fill;
    }
    return buf;
}


void kmain(void) {
    // initialize the bss section with 0s
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    for (;;); // spin forever
}

/*
The boot function is placed at .text.boot since
the first section in the linked executable is .text.boot
*/
__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n" // place stack top in stack pointer
        "j kmain\n" // jump to kmain function
        :
        : [stack_top] "r" (__stack_top)
    );
}