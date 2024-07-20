#include "cheat_to_code.h"

#include "third_party/nugget/common/util/encoder.hh"
#include "syscalls.h"

using namespace Mips;

template <typename T>
T getValue(const uint8_t *&buffer)
{
    T ret = 0;
    for (int i = 0; i < sizeof(T); i++)
    {
        T v = *buffer++;
        ret |= (v << (8 * i));
    }
    return ret;
}

void appendCode(uint32_t code, uint32_t *&ptr, uint32_t &size)
{
    if (ptr)
        *ptr++ = code;
    size += 4;
}

static constexpr unsigned NUM_REGS = 16;

static constexpr Encoder::Reg s_mapping[NUM_REGS + 1] = {
    Encoder::Reg::T0,
    Encoder::Reg::T1,
    Encoder::Reg::T2,
    Encoder::Reg::T3,
    Encoder::Reg::T4,
    Encoder::Reg::T5,
    Encoder::Reg::T6,
    Encoder::Reg::T7,
    Encoder::Reg::T8,
    Encoder::Reg::T9,
    Encoder::Reg::A0,
    Encoder::Reg::A1,
    Encoder::Reg::A2,
    Encoder::Reg::A3,
    Encoder::Reg::V0,
    Encoder::Reg::V1,
    Encoder::Reg::R0,
};

struct RegVal
{
    uint32_t value = 0;
    bool set = 0;
    bool locked = 0;
};

static constexpr int findEmptySlot(RegVal regs[NUM_REGS])
{
    for (unsigned i = 0; i < NUM_REGS; i++)
    {
        if (!regs[i].set)
            return i;
    }
    return NUM_REGS;
}

static constexpr unsigned findSlot(RegVal regs[NUM_REGS])
{
    unsigned ret = findEmptySlot(regs);
    if (ret != NUM_REGS)
        return ret;
    for (unsigned i = 0; i < NUM_REGS; i++)
    {
        if (!regs[i].locked)
            return i;
    }
    return NUM_REGS; // should probably abort() here instead
}

static unsigned findOffset(RegVal regs[NUM_REGS], uint32_t value, int16_t &distance)
{
    for (unsigned i = 0; i < NUM_REGS; i++)
    {
        if (regs[i].set)
        {
            int32_t d = value - regs[i].value;
            if ((-32768 < d) && (d < 32768))
            {
                distance = d;
                return i;
            }
        }
    }
    return NUM_REGS;
}

static constexpr unsigned loadValue(RegVal regs[NUM_REGS], uint32_t value, uint32_t *&ptr, uint32_t &size)
{
    // zero ?
    if (value == 0)
        return NUM_REGS;
    // same value ?
    for (unsigned i = 0; i < NUM_REGS; i++)
    {
        if (regs[i].set)
        {
            if (regs[i].value == value)
            {
                return i;
            }
        }
    }
    // can use an lui?
    if ((value & 0xffff) == 0)
    {
        unsigned slot = findSlot(regs);
        appendCode(Encoder::lui(s_mapping[slot], value >> 16), ptr, size);
        regs[slot].set = true;
        regs[slot].value = value;
        return slot;
    }
    // can use an ori ?
    if ((value & 0xffff0000) == 0)
    {
        unsigned slot = findSlot(regs);
        appendCode(Encoder::ori(s_mapping[slot], Encoder::Reg::R0, value), ptr, size);
        regs[slot].set = true;
        regs[slot].value = value;
        return slot;
    }
    // add distance ?
    int16_t d = 0;
    unsigned src = findOffset(regs, value, d);
    if (src != NUM_REGS)
    {
        int slot = findEmptySlot(regs);
        if (slot == -1)
            slot = src;
        appendCode(Encoder::addiu(s_mapping[slot], s_mapping[src], d), ptr, size);
        regs[slot].set = true;
        regs[slot].value = value;
        return slot;
    }

    // fallback to full load
    unsigned slot = findSlot(regs);
    appendCode(Encoder::lui(s_mapping[slot], value >> 16), ptr, size);
    appendCode(Encoder::ori(s_mapping[slot], s_mapping[slot], value & 0xffff), ptr, size);
    regs[slot].set = true;
    regs[slot].value = value;
    return slot;
}

static void lockReg(RegVal regs[NUM_REGS], unsigned slot)
{
    if (slot == NUM_REGS)
        return;
    regs[slot].locked = true;
}

static void unlockReg(RegVal regs[NUM_REGS], unsigned slot)
{
    if (slot == NUM_REGS)
        return;
    regs[slot].locked = false;
}

unsigned cheat_to_code(const void *cheat_, unsigned size, void *code_)
{
    const uint8_t *cheat = (const uint8_t *)cheat_;
    uint32_t *code = (uint32_t *)code_;
    RegVal regs[NUM_REGS];

    if (size == 0)
        return 0;
    if (size % 6)
        return 0;
    size /= 6;

    uint32_t ret = 0;
    uint32_t condition = 0;

    auto applyCondition = [&](unsigned oldRet, unsigned count) mutable
    {
        if (condition)
        {
            uint32_t c = condition | (count + 1);
            if (oldRet != ret)
            {
                if (code)
                {
                    appendCode(*(code - 1), code, ret);
                    *(code - 2) = c;
                }
                else
                {
                    ret += 4;
                }
            }
            else
            {
                appendCode(c, code, ret);
                appendCode(Encoder::nop(), code, ret);
            }
            condition = 0;
        }
    };

    while (size--)
    {
        uint32_t a = getValue<uint32_t>(cheat);
        uint16_t v = getValue<uint16_t>(cheat);

        switch (a >> 24)
        {
        case 0x30:
        {
            unsigned oldRet = ret;
            uint32_t addr = 0x80000000 | (a & 0xffffff);
            int16_t d = 0;
            unsigned src = findOffset(regs, addr, d);
            if (src == NUM_REGS)
            {
                src = loadValue(regs, addr, code, ret);
                d = 0;
            }
            lockReg(regs, src);
            unsigned dst = loadValue(regs, v, code, ret);
            unlockReg(regs, src);

            applyCondition(oldRet, 1);
            appendCode(Encoder::sh(s_mapping[dst], d, s_mapping[src]), code, ret);
        }
        break;
        case 0x80:
        {
            unsigned oldRet = ret;
            uint32_t addr = 0x80000000 | (a & 0xffffff);
            int16_t d = 0;
            unsigned src = findOffset(regs, addr, d);
            if (src == NUM_REGS)
            {
                src = loadValue(regs, addr, code, ret);
                d = 0;
            }
            lockReg(regs, src);
            unsigned dst = loadValue(regs, v, code, ret);
            unlockReg(regs, src);

            applyCondition(oldRet, 1);
            appendCode(Encoder::sh(s_mapping[dst], d, s_mapping[src]), code, ret);
        }
        break;
        case 0xd0:
        {
            uint32_t addr = 0x80000000 | (a & 0xffffff);
            int16_t d = 0;
            unsigned src = findOffset(regs, addr, d);
            if (src == NUM_REGS)
            {
                src = loadValue(regs, addr, code, ret);
                d = 0;
            }
            appendCode(Encoder::lh(Encoder::Reg::AT, d, s_mapping[src]), code, ret);
            unsigned oldRet = ret;
            unsigned dst = loadValue(regs, v, code, ret);
            if (ret == oldRet)
                appendCode(Encoder::nop(), code, ret);
            appendCode(Encoder::subu(Encoder::Reg::AT, Encoder::Reg::AT, s_mapping[dst]), code, ret);
            condition = Encoder::beq(Encoder::Reg::AT, Encoder::Reg::R0, 0);
        }
        break;
        case 0xd1:
        {
            uint32_t addr = 0x80000000 | (a & 0xffffff);
            int16_t d = 0;
            unsigned src = findOffset(regs, addr, d);
            if (src == NUM_REGS)
            {
                src = loadValue(regs, addr, code, ret);
                d = 0;
            }
            appendCode(Encoder::lh(Encoder::Reg::AT, d, s_mapping[src]), code, ret);
            unsigned oldRet = ret;
            unsigned dst = loadValue(regs, v, code, ret);
            if (ret == oldRet)
                appendCode(Encoder::nop(), code, ret);
            appendCode(Encoder::subu(Encoder::Reg::AT, Encoder::Reg::AT, s_mapping[dst]), code, ret);
            condition = Encoder::bne(Encoder::Reg::AT, Encoder::Reg::R0, 0);
        }
        break;
        case 0xd2:
        {
            uint32_t addr = 0x80000000 | (a & 0xffffff);
            int16_t d = 0;
            unsigned src = findOffset(regs, addr, d);
            if (src == NUM_REGS)
            {
                src = loadValue(regs, addr, code, ret);
                d = 0;
            }
            appendCode(Encoder::lh(Encoder::Reg::AT, d, s_mapping[src]), code, ret);
            unsigned oldRet = ret;
            unsigned dst = loadValue(regs, v, code, ret);
            if (ret == oldRet)
                appendCode(Encoder::nop(), code, ret);
            appendCode(Encoder::slt(Encoder::Reg::AT, Encoder::Reg::AT, s_mapping[dst]), code, ret);
            condition = Encoder::bne(Encoder::Reg::AT, Encoder::Reg::R0, 0);
        }
        break;
        case 0xd3:
        {
            uint32_t addr = 0x80000000 | (a & 0xffffff);
            int16_t d = 0;
            unsigned src = findOffset(regs, addr, d);
            if (src == NUM_REGS)
            {
                src = loadValue(regs, addr, code, ret);
                d = 0;
            }
            appendCode(Encoder::lh(Encoder::Reg::AT, d, s_mapping[src]), code, ret);
            unsigned oldRet = ret;
            unsigned dst = loadValue(regs, v, code, ret);
            if (ret == oldRet)
                appendCode(Encoder::nop(), code, ret);
            appendCode(Encoder::slt(Encoder::Reg::AT, s_mapping[dst], Encoder::Reg::AT), code, ret);
            condition = Encoder::bne(Encoder::Reg::AT, Encoder::Reg::R0, 0);
        }
        break;
        case 0xe0:
        {
            uint32_t addr = 0x80000000 | (a & 0xffffff);
            int16_t d = 0;
            unsigned src = findOffset(regs, addr, d);
            if (src == NUM_REGS)
            {
                src = loadValue(regs, addr, code, ret);
                d = 0;
            }
            appendCode(Encoder::lb(Encoder::Reg::AT, d, s_mapping[src]), code, ret);
            unsigned oldRet = ret;
            unsigned dst = loadValue(regs, v, code, ret);
            if (ret == oldRet)
                appendCode(Encoder::nop(), code, ret);
            appendCode(Encoder::subu(Encoder::Reg::AT, Encoder::Reg::AT, s_mapping[dst]), code, ret);
            condition = Encoder::beq(Encoder::Reg::AT, Encoder::Reg::R0, 0);
        }
        break;
        case 0xe1:
        {
            uint32_t addr = 0x80000000 | (a & 0xffffff);
            int16_t d = 0;
            unsigned src = findOffset(regs, addr, d);
            if (src == NUM_REGS)
            {
                src = loadValue(regs, addr, code, ret);
                d = 0;
            }
            appendCode(Encoder::lb(Encoder::Reg::AT, d, s_mapping[src]), code, ret);
            unsigned oldRet = ret;
            unsigned dst = loadValue(regs, v, code, ret);
            if (ret == oldRet)
                appendCode(Encoder::nop(), code, ret);
            appendCode(Encoder::subu(Encoder::Reg::AT, Encoder::Reg::AT, s_mapping[dst]), code, ret);
            condition = Encoder::bne(Encoder::Reg::AT, Encoder::Reg::R0, 0);
        }
        break;
        case 0xe2:
        {
            uint32_t addr = 0x80000000 | (a & 0xffffff);
            int16_t d = 0;
            unsigned src = findOffset(regs, addr, d);
            if (src == NUM_REGS)
            {
                src = loadValue(regs, addr, code, ret);
                d = 0;
            }
            appendCode(Encoder::lb(Encoder::Reg::AT, d, s_mapping[src]), code, ret);
            unsigned oldRet = ret;
            unsigned dst = loadValue(regs, v, code, ret);
            if (ret == oldRet)
                appendCode(Encoder::nop(), code, ret);
            appendCode(Encoder::slt(Encoder::Reg::AT, Encoder::Reg::AT, s_mapping[dst]), code, ret);
            condition = Encoder::bne(Encoder::Reg::AT, Encoder::Reg::R0, 0);
        }
        break;
        case 0xe3:
        {
            uint32_t addr = 0x80000000 | (a & 0xffffff);
            int16_t d = 0;
            unsigned src = findOffset(regs, addr, d);
            if (src == NUM_REGS)
            {
                src = loadValue(regs, addr, code, ret);
                d = 0;
            }
            appendCode(Encoder::lb(Encoder::Reg::AT, d, s_mapping[src]), code, ret);
            unsigned oldRet = ret;
            unsigned dst = loadValue(regs, v, code, ret);
            if (ret == oldRet)
                appendCode(Encoder::nop(), code, ret);
            appendCode(Encoder::slt(Encoder::Reg::AT, s_mapping[dst], Encoder::Reg::AT), code, ret);
            condition = Encoder::bne(Encoder::Reg::AT, Encoder::Reg::R0, 0);
        }
        break;
        case 0x10:
        {
            unsigned oldRet = ret;
            uint32_t addr = 0x80000000 | (a & 0xffffff);
            int16_t d = 0;
            unsigned src = findOffset(regs, addr, d);
            if (src == NUM_REGS)
            {
                src = loadValue(regs, addr, code, ret);
                d = 0;
            }
            applyCondition(oldRet, 4);
            appendCode(Encoder::lh(Encoder::Reg::AT, d, s_mapping[src]), code, ret);
            appendCode(Encoder::nop(), code, ret);
            appendCode(Encoder::addiu(Encoder::Reg::AT, Encoder::Reg::AT, v), code, ret);
            appendCode(Encoder::sh(Encoder::Reg::AT, d, s_mapping[src]), code, ret);
        }
        break;
        case 0x11:
        {
            unsigned oldRet = ret;
            uint32_t addr = 0x80000000 | (a & 0xffffff);
            int16_t d = 0;
            unsigned src = findOffset(regs, addr, d);
            if (src == NUM_REGS)
            {
                src = loadValue(regs, addr, code, ret);
                d = 0;
            }
            applyCondition(oldRet, 4);
            appendCode(Encoder::lh(Encoder::Reg::AT, d, s_mapping[src]), code, ret);
            appendCode(Encoder::nop(), code, ret);
            appendCode(Encoder::addiu(Encoder::Reg::AT, Encoder::Reg::AT, -v), code, ret);
            appendCode(Encoder::sh(Encoder::Reg::AT, d, s_mapping[src]), code, ret);
        }
        break;
        case 0x20:
        {
            unsigned oldRet = ret;
            uint32_t addr = 0x80000000 | (a & 0xffffff);
            int16_t d = 0;
            unsigned src = findOffset(regs, addr, d);
            if (src == NUM_REGS)
            {
                src = loadValue(regs, addr, code, ret);
                d = 0;
            }
            applyCondition(oldRet, 4);
            appendCode(Encoder::lb(Encoder::Reg::AT, d, s_mapping[src]), code, ret);
            appendCode(Encoder::nop(), code, ret);
            appendCode(Encoder::addiu(Encoder::Reg::AT, Encoder::Reg::AT, v), code, ret);
            appendCode(Encoder::sb(Encoder::Reg::AT, d, s_mapping[src]), code, ret);
        }
        break;
        case 0x21:
        {
            unsigned oldRet = ret;
            uint32_t addr = 0x80000000 | (a & 0xffffff);
            int16_t d = 0;
            unsigned src = findOffset(regs, addr, d);
            if (src == NUM_REGS)
            {
                src = loadValue(regs, addr, code, ret);
                d = 0;
            }
            applyCondition(oldRet, 4);
            appendCode(Encoder::lb(Encoder::Reg::AT, d, s_mapping[src]), code, ret);
            appendCode(Encoder::nop(), code, ret);
            appendCode(Encoder::addiu(Encoder::Reg::AT, Encoder::Reg::AT, -v), code, ret);
            appendCode(Encoder::sb(Encoder::Reg::AT, d, s_mapping[src]), code, ret);
        }
        break;
        }
    }

    if (condition)
        return 0;

    if (code)
    {
        appendCode(*(code - 1), code, ret);
        *(code - 2) = Encoder::jr(Encoder::Reg::RA);
    }
    else
    {
        ret += 4;
    }

    return ret;
}
