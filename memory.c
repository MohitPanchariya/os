#include "memory.h"
#include "types.h"
#include "kernel.h"

extern char __free_ram[], __free_ram_end[];

void* memset(void* buf, char fill, size_t buf_size) {
    uint8_t* p = (uint8_t* ) buf;
    while(buf_size--) {
        *p++ = fill;
    }
    return buf;
}

void* memcpy(void* dst, const void* src, size_t n) {
    uint8_t* d = (uint8_t*) dst;
    const uint8_t* s = (const uint8_t*) src;

    while(n--) {
        *d++ = *s++;
    }
    return dst;
}

paddr_t allocate_pages(uint32_t n) {
    // TODO: make a better allocator
    // This linear allocate can't deallocate memory
    static paddr_t next_free = (paddr_t) __free_ram;
    // starting address of allocated memory
    paddr_t start = next_free;
    next_free += (paddr_t) n * PAGE_SIZE;

    if (next_free > (paddr_t) __free_ram_end) {
        PANIC("OUT OF MEMORY");
    }

    // uncomment to debug memory issues
    // memset((void*) start, 0, n * PAGE_SIZE);

    return start;
}

// map a virtual address to a physical address
void map_page(uint32_t* pg_table1, uint32_t vaddr, paddr_t paddr, uint32_t flags) {
    if (!is_aligned(vaddr, PAGE_SIZE))
        PANIC("unaligned vaddr %x", vaddr);

    if (!is_aligned(paddr, PAGE_SIZE))
        PANIC("unaligned paddr %x", paddr);

    // based on the SV32 paging schema
    uint32_t vpn1 = (uint32_t) ((vaddr >> 22) & 0x3ff);
    
    // create the first level page table entry
    if((pg_table1[vpn1] & PAGE_V) == 0) {
        uint32_t pt_addr = allocate_pages(1);
        // PPNs (physical page number) are stored in page tables
        pg_table1[vpn1] = ((pt_addr / PAGE_SIZE) << 10) | PAGE_V;
    }

    uint32_t vpn0 = (uint32_t) ((vaddr >> 12) & 0x3ff);
    // start of the second level page table
    uint32_t* pg_table0 = (uint32_t*) ((pg_table1[vpn1] >> 10) * PAGE_SIZE);
    pg_table0[vpn0] = (uint32_t) ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}