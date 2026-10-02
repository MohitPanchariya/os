#pragma once

#include "types.h"

#define MAX_PROCS 8
#define PROC_UNUSED 0
#define PROC_RUNNABLE 1
#define PROC_EXITED 2

// base virtual address of application binary.
// matches the start address in the user linker script
#define USER_BASE 0x1000000

// enable hardware interrupts
#define SSTATUS_SPIE (1 << 5)

struct process {
    int pid; 
    // PROC_UNUSED or PROC_RUNNABLE
    int state;
    // pointer to kernel stack
    vaddr_t sp;
    // pointer to page table
    uint32_t* pg_table;
    // kernel stack
    uint8_t stack[8192];
};

// currently running process
extern struct process* current_proc;
// process to switch to, if there are no runnable processes
extern struct process* idle_proc;

void switch_context(uint32_t* prev_sp, uint32_t* next_sp);
struct process* create_process(const void* image, size_t image_size);
void yield(void);
void proc_init(void);