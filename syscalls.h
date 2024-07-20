#pragma once

#include <stdint.h>

static __attribute__((always_inline)) uint32_t *syscall_getB0Table()
{
    register int n asm("t1") = 0x57;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((uint32_t * (*)())0xb0)();
}
