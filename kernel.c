#include "io.h"
#include "types.h"
#include "memory.h"
#include "kernel.h"
#include "process.h"

extern char __stack_top[], __bss[], __bss_end[];


void handle_trap(struct trap_frame* f) {
    // cause of crash
    uint32_t scause = READ_CSR(scause);
    // additional info on crash
    uint32_t stval = READ_CSR(stval);
    // pc pointing to the point that caused the crash
    uint32_t user_pc = READ_CSR(sepc);

    PANIC("trap triggered. scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
}

__attribute__((naked))
__attribute__((aligned(4)))
void exception_handler(void) {
    __asm__ __volatile__(
        // swap sscratch and sp
        // store kernel stack pointer in sp
        // and contents of sp in sscratch
        "csrrw sp, sscratch, sp\n" // save stack pointer to sscratch register
        "addi sp, sp, -4 * 31\n" // allocate space for the trap_frame struct
        // write out the general-purpose registers to stack
        "sw ra,  4 * 0(sp)\n"
        "sw gp,  4 * 1(sp)\n"
        "sw tp,  4 * 2(sp)\n"
        "sw t0,  4 * 3(sp)\n"
        "sw t1,  4 * 4(sp)\n"
        "sw t2,  4 * 5(sp)\n"
        "sw t3,  4 * 6(sp)\n"
        "sw t4,  4 * 7(sp)\n"
        "sw t5,  4 * 8(sp)\n"
        "sw t6,  4 * 9(sp)\n"
        "sw a0,  4 * 10(sp)\n"
        "sw a1,  4 * 11(sp)\n"
        "sw a2,  4 * 12(sp)\n"
        "sw a3,  4 * 13(sp)\n"
        "sw a4,  4 * 14(sp)\n"
        "sw a5,  4 * 15(sp)\n"
        "sw a6,  4 * 16(sp)\n"
        "sw a7,  4 * 17(sp)\n"
        "sw s0,  4 * 18(sp)\n"
        "sw s1,  4 * 19(sp)\n"
        "sw s2,  4 * 20(sp)\n"
        "sw s3,  4 * 21(sp)\n"
        "sw s4,  4 * 22(sp)\n"
        "sw s5,  4 * 23(sp)\n"
        "sw s6,  4 * 24(sp)\n"
        "sw s7,  4 * 25(sp)\n"
        "sw s8,  4 * 26(sp)\n"
        "sw s9,  4 * 27(sp)\n"
        "sw s10, 4 * 28(sp)\n"
        "sw s11, 4 * 29(sp)\n"

        /*
        The reason sscratch is read into a0, and then
        written to 4 * 30(sp) is because the sw instruction
        doesn't directly work with the sscratch register
        sw sscratch, 4 * 30(sp) doesn't work
        */
        "csrr a0, sscratch\n"
        // the trap_frame's last member is the stack pointer
        // store the original stack pointer at the end of the
        // trap_frame
        "sw a0, 4 * 30(sp)\n"

        // TODO: check if the following two instructions
        // serve the smae purpose as the following 3 instrutions
        // addi sp, sp, 4 * 31
        // mv sp, a0

        // reset the kernel stack pointer
        "addi a0, sp, 4 * 31\n"
        "csrw sscratch, a0\n"

        // copy stack pointer into a0 register
        // a0 holds the first parameter for a
        // function being called
        "mv a0, sp\n"

        // invoke the trap handler function
        "call handle_trap\n"

        // restore register values
        "lw ra,  4 * 0(sp)\n"
        "lw gp,  4 * 1(sp)\n"
        "lw tp,  4 * 2(sp)\n"
        "lw t0,  4 * 3(sp)\n"
        "lw t1,  4 * 4(sp)\n"
        "lw t2,  4 * 5(sp)\n"
        "lw t3,  4 * 6(sp)\n"
        "lw t4,  4 * 7(sp)\n"
        "lw t5,  4 * 8(sp)\n"
        "lw t6,  4 * 9(sp)\n"
        "lw a0,  4 * 10(sp)\n"
        "lw a1,  4 * 11(sp)\n"
        "lw a2,  4 * 12(sp)\n"
        "lw a3,  4 * 13(sp)\n"
        "lw a4,  4 * 14(sp)\n"
        "lw a5,  4 * 15(sp)\n"
        "lw a6,  4 * 16(sp)\n"
        "lw a7,  4 * 17(sp)\n"
        "lw s0,  4 * 18(sp)\n"
        "lw s1,  4 * 19(sp)\n"
        "lw s2,  4 * 20(sp)\n"
        "lw s3,  4 * 21(sp)\n"
        "lw s4,  4 * 22(sp)\n"
        "lw s5,  4 * 23(sp)\n"
        "lw s6,  4 * 24(sp)\n"
        "lw s7,  4 * 25(sp)\n"
        "lw s8,  4 * 26(sp)\n"
        "lw s9,  4 * 27(sp)\n"
        "lw s10, 4 * 28(sp)\n"
        "lw s11, 4 * 29(sp)\n"
        "lw sp,  4 * 30(sp)\n"
        "sret\n"
    );
}


void delay(void) {
    for(int i = 0; i < 30000000; i++) {
        __asm__ __volatile__("nop");
    }
}

struct process* proc_a;
struct process* proc_b;

void proc_a_entry(void) {
    printf("starting process A\n");
    while (1) {
        putchar('A');
        yield();
    }
}

void proc_b_entry(void) {
    printf("starting process B\n");
    while (1) {
        putchar('B');
        yield();
    }
}

void kmain(void) {
    // initialize the bss section with 0s
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    // setup exception handler
    // In riscv32, stvec stores the address of the exception
    // handler for U/S mode exceptions
    WRITE_CSR(stvec, (uint32_t) exception_handler);

    printf("Kernel says %s\n", "Hello!");

    proc_init();

    proc_a = create_process((uint32_t) proc_a_entry);
    proc_b = create_process((uint32_t) proc_b_entry);

    yield();
    PANIC("Returned to idle process");

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