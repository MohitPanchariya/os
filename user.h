#pragma once

#include "io.h"

__attribute__((noreturn)) void exit(void);
void putchar(char ch);

int syscall(int syscallno, int arg0, int arg1, int arg2);