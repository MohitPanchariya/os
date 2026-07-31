#include "sbi.h"
#include "io.h"

void putchar(char ch) {
    // calls Console Putchar exposed by OpenSBI
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}