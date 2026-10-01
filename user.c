#include "user.h"
#include "syscall.h"

extern char __stack_top[];

void putchar(char ch) {
    syscall(SYS_PUTCHAR, ch, 0, 0);
    return;
}

int getchar(void) {
    syscall(SYS_GETCHAR, 0, 0, 0);
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

// userland function to invoke kernel functions
int syscall(int syscallno, int arg0, int arg1, int arg2) {
    register int a0 __asm__("a0") = arg0;
    register int a1 __asm__("a1") = arg1;
    register int a2 __asm__("a2") = arg2;
    register int a3 __asm__("a3") = syscallno;

    __asm__ __volatile__("ecall"
                        : "=r"(a0)
                        : "r"(a0), "r"(a1), "r"(a2), "r"(a3)
                        : "memory");

    // value returned by kernel
    return a0;
}