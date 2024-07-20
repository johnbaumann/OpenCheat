#pragma once

#include "third_party/nugget/common/util/encoder.hh"

namespace OpenCheat
{
    using namespace Mips::Encoder;

    void installKernelPatch();

    [[noreturn]] void reboot();
}