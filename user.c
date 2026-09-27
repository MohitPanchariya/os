#include "user.h"

extern char __stack_top[];

void putchar(char ch) {
    // TODO
    return;
}

__attribute__((noreturn)) void exit(void) {
    for(;;);
}

// defined as the entry point in the linker script
__attribute__((section(".text.start")))
__attribute__((naked))
void start(void) {
    // setup stack, call main, and finally call exit
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n"
        "call main\n"
        "call exit\n"
        :
        : [stack_top] "r" (__stack_top)
    );
}