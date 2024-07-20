#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void appendCode(uint32_t code, uint32_t *&ptr, uint32_t &size);
unsigned cheat_to_code(const void* cheat, unsigned size, void* code);

#ifdef __cplusplus
}
#endif
