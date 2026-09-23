#pragma once
#include "types.h"

void* memset(void* buf, char c, size_t n);
void* memcpy(void* dst, const void* src, size_t n);
paddr_t allocate_pages(uint32_t n);
void map_page(uint32_t* pg_table1, uint32_t vaddr, paddr_t paddr, uint32_t flags);

#define PAGE_SIZE 4096

#define SATP_SV32 (1u << 31) // SV32 is the paging mode
#define PAGE_V    (1 << 0)   // "Valid" bit (entry is enabled)
#define PAGE_R    (1 << 1)   // Readable
#define PAGE_W    (1 << 2)   // Writable
#define PAGE_X    (1 << 3)   // Executable
#define PAGE_U    (1 << 4)   // User (accessible in user mode)