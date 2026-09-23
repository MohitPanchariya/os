#include "process.h"
#include "types.h"
#include "kernel.h"
#include "memory.h"

struct process processes[MAX_PROCS];
// currently running process
struct process* current_proc;
// process to switch to, if there are no runnable processes
struct process* idle_proc;

extern char __kernel_base[];
extern char __free_ram_end[];

__attribute__((naked))
void switch_context(uint32_t* prev_sp, uint32_t* next_sp) {
    __asm__ __volatile__(
        // save callee saved registers into the kernel stack
        // of the previous process. There are 13 callee saved
        // registers in riscv
        "addi sp, sp, -13 * 4\n"
        "sw ra,  0  * 4(sp)\n"
        "sw s0,  1  * 4(sp)\n"
        "sw s1,  2  * 4(sp)\n"
        "sw s2,  3  * 4(sp)\n"
        "sw s3,  4  * 4(sp)\n"
        "sw s4,  5  * 4(sp)\n"
        "sw s5,  6  * 4(sp)\n"
        "sw s6,  7  * 4(sp)\n"
        "sw s7,  8  * 4(sp)\n"
        "sw s8,  9  * 4(sp)\n"
        "sw s9,  10 * 4(sp)\n"
        "sw s10, 11 * 4(sp)\n"
        "sw s11, 12 * 4(sp)\n"

        // switch the stack pointer to the next processes stack pointer
        "sw sp, (a0)\n" // *prev_sp = sp
        "lw sp, (a1)\n" // sp = *next_sp

        // restore callee saved registers of the next process
        "lw ra,  0  * 4(sp)\n"
        "lw s0,  1  * 4(sp)\n"
        "lw s1,  2  * 4(sp)\n"
        "lw s2,  3  * 4(sp)\n"
        "lw s3,  4  * 4(sp)\n"
        "lw s4,  5  * 4(sp)\n"
        "lw s5,  6  * 4(sp)\n"
        "lw s6,  7  * 4(sp)\n"
        "lw s7,  8  * 4(sp)\n"
        "lw s8,  9  * 4(sp)\n"
        "lw s9,  10 * 4(sp)\n"
        "lw s10, 11 * 4(sp)\n"
        "lw s11, 12 * 4(sp)\n"
        "addi sp, sp, 13 * 4\n"
        "ret\n"
    );
}

struct process* create_process(uint32_t process_start) {
    struct process* proc = NULL;
    int i;
    for(i = 0; i < MAX_PROCS; i++) {
        if(processes[i].state == PROC_UNUSED) {
            proc = &processes[i];
            break;
        }
    }

    if (!proc) {
        PANIC("no free processes");
    }

    uint32_t* page_table = (uint32_t*) allocate_pages(1);

    // map all physical pages to the same address as the virtual pages
    for(paddr_t paddr = (paddr_t) __kernel_base; paddr < (paddr_t) __free_ram_end; paddr += PAGE_SIZE) {
        map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);
    }

    proc->pid = i + 1;
    proc->state = PROC_RUNNABLE;

    // initialise the kernel stack
    uint32_t *sp = (uint32_t *) &proc->stack[sizeof(proc->stack)];
    *--sp = 0;                           // s11
    *--sp = 0;                           // s10
    *--sp = 0;                           // s9
    *--sp = 0;                           // s8
    *--sp = 0;                           // s7
    *--sp = 0;                           // s6
    *--sp = 0;                           // s5
    *--sp = 0;                           // s4
    *--sp = 0;                           // s3
    *--sp = 0;                           // s2
    *--sp = 0;                           // s1
    *--sp = 0;                           // s0
    *--sp = (uint32_t) process_start;    // ra

    proc->sp = (uint32_t) sp;
    proc->pg_table = page_table;
    return proc;
}

// yield control to scheduler
void yield(void) {
    struct process* next = idle_proc;
    for (int i = 0; i < MAX_PROCS; i++) {
        struct process* process = &processes[(current_proc->pid + i) % MAX_PROCS];
        if (process->state == PROC_RUNNABLE && process->pid > 0) {
            next = process;
            break;
        }
    }

    // don't context switch if there isn't another runnable process
    if (next == current_proc) {
        return;
    }

    // save the kernel stack pointer of the next process to be executed
    // when an exception occurs, the stack pointer is restored in the
    // exception handler
    __asm__ __volatile__(
        "sfence.vma\n"
        "csrw satp, %[satp]\n"
        "sfence.vma\n"
        "csrw sscratch, %[sscratch]\n"
        :
        : [satp] "r" (SATP_SV32 | ((uint32_t) next->pg_table / PAGE_SIZE)),
        [sscratch] "r" ((uint32_t) &next->stack[sizeof(next->stack)])
    );

    struct process* prev = current_proc;
    current_proc = next;
    switch_context(&prev->sp, &next->sp);
}

void proc_init(void) {
    idle_proc = create_process((uint32_t) NULL);
    idle_proc->pid = 0;
    current_proc = idle_proc;
}