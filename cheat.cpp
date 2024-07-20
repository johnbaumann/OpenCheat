#include "cheat.hh"

#include <stdint.h>

#include "third_party/nugget/common/syscalls/syscalls.h"
#include "third_party/nugget/common/util/encoder.hh"
#include "third_party/nugget/psyqo/kernel.hh"

#include "cheat_to_code.h"
#include "syscalls.h"

namespace OpenCheat
{
    static const uint32_t shellReplacement[] = {
        addiu(Reg::R0, Reg::R0, 0),
        j(reinterpret_cast<uintptr_t>(installKernelPatch)),
        nop(),
    };

    void installKernelPatch()
    {
        uint8_t code_in[][6] = {
            {0x96, 0x01, 0x08, 0x80, 0x0C, 0x00}, // Ridge Racer - Have Black Car	80080196 000C
        };

        uint32_t code_out[64];
        uint32_t *code_ptr = code_out;
        syscall_memset(code_out, 0, sizeof(code_out));

        uint32_t code_size = 0;

        uint32_t *b0table = syscall_getB0Table();
        
        uint32_t rfeHandler = b0table[0x17]; // Get original RFE handler, set it as the return address
        appendCode(Mips::Encoder::lui(Mips::Encoder::Reg::RA, rfeHandler >> 16), code_ptr, code_size);
        appendCode(Mips::Encoder::ori(Mips::Encoder::Reg::RA, Mips::Encoder::Reg::RA, rfeHandler & 0xFFFF), code_ptr, code_size);

        code_size += cheat_to_code(code_in, sizeof(code_in), code_ptr);

        if(code_size > sizeof(code_out))
        {
            code_size = sizeof(code_out);
        }

        // Copy the output to a code cave, to-do: OpenBIOS compatibility - maybe request a code cave from OpenBIOS?
        constexpr uint32_t kernelPatchDest = 0xc380;
        __builtin_memcpy(reinterpret_cast<void *>(kernelPatchDest), code_out,
                         code_size);

        b0table[0x17] = kernelPatchDest; // Set the new RFE handler
    }

    [[noreturn]] void reboot()
    {
        psyqo::Kernel::fastEnterCriticalSection();
        constexpr uint32_t breakHandler[] = {
            mtc0(Reg::R0, 7),
            jr(Reg::RA),
            rfe(),
        };

        __builtin_memcpy(reinterpret_cast<void *>(0xA0000040), breakHandler,
                         sizeof(breakHandler));
        __builtin_memcpy(reinterpret_cast<void *>(0x80030000), shellReplacement,
                         sizeof(shellReplacement));
        asm(R"(
            lui   $t0, 0x8003
            addiu $t1, $0, -1
            mtc0  $0, $7
            lui   $ra, 0xbfc0
            mtc0  $t0, $5
            addiu $ra, 0x0390
            mtc0  $t1, $9
            li    $t0, 0xa0
            lui   $t1, 0xca80
            mtc0  $t1, $7
            li    $t1, 0x44
            jr    $t0
        )");
        while (1)
        {
            asm("");
        }

        __builtin_unreachable();
    }
} // namespace OpenCheat