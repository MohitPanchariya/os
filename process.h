#pragma once

#include "types.h"

#define MAX_PROCS 8
#define PROC_UNUSED 0
#define PROC_RUNNABLE 1

struct process {
    int pid; 
    // PROC_UNUSED or PROC_RUNNABLE
    int state;
    // pointer to kernel stack
    vaddr_t sp;
    // kernel stack
    uint8_t stack[8192];
};

void switch_context(uint32_t* prev_sp, uint32_t* next_sp);
struct process* create_process(uint32_t process_start);
void yield(void);
void proc_init(void);