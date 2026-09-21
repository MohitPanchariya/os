#include "memory.h"
#include "types.h"
#include "kernel.h"

#define PAGE_SIZE 4096

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