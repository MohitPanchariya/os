#pragma once
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef uint32_t size_t;
// paddr_t represents a physical address type
typedef uint32_t paddr_t;
// vaddr_t represents a virtual address type
typedef uint32_t vaddr_t;

#define true  1
#define false 0
#define NULL  ((void *) 0)
#define align_up(value, align)   __builtin_align_up(value, align)
#define is_aligned(value, align) __builtin_is_aligned(value, align)
// offset of a member within a struct
#define offsetof(type, member)   __builtin_offsetof(type, member)
