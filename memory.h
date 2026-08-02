#pragma once
#include "types.h"

void* memset(void* buf, char c, size_t n);
void* memcpy(void* dst, const void* src, size_t n);
paddr_t allocate_pages(uint32_t n);