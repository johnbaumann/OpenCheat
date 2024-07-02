#include <stdint.h>

#include "third_party/nugget/common/syscalls/syscalls.h"
#include "third_party/nugget/common/util/encoder.hh"
#include "third_party/nugget/psyqo/application.hh"
#include "third_party/nugget/psyqo/font.hh"
#include "third_party/nugget/psyqo/gpu.hh"
#include "third_party/nugget/psyqo/scene.hh"

#include "cheat_to_code.h"

namespace
{

    using namespace Mips::Encoder;
    union Cheat
    {
        uint8_t bytes[6];
        struct
        {
            uint32_t address;
            uint16_t value;
        };
    };

    // A PSYQo software needs to declare one \`Application\` object.
    // This is the one we're going to do for our hello world.
    class Hello final : public psyqo::Application
    {

        void prepare() override;
        void createScene() override;

    public:
        psyqo::Font<> m_font;
    };

    // And we need at least one scene to be created.
    // This is the one we're going to do for our hello world.
    class HelloScene final : public psyqo::Scene
    {
        void frame() override;

        // We'll have some simple animation going on, so we
        // need to keep track of our state here.
        uint8_t m_anim = 0;
        bool m_direction = true;
    };

    // We're instantiating the two objects above right now.
    Hello hello;
    HelloScene helloScene;

} // namespace

void installKernelPatch()
{
    uint8_t code_in[][6] = {
        {0x96, 0x01, 0x08, 0x80, 0x0C, 0x00}, // Ridge Racer - Have Black Car	80080196 000C
        };

    uint32_t code_out[64];
    syscall_memset(code_out, 0, sizeof(code_out));
    unsigned code_size = cheat_to_code(code_in, sizeof(code_in), &code_out); // Also sets RA = original rfe handler

    constexpr uint32_t kernelPatchDest = 0xc380;
    __builtin_memcpy(reinterpret_cast<void *>(kernelPatchDest), code_out,
                     code_size);
    
    // GetB0Table
    register int n asm("t1") = 0x57;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    uint32_t *b0table = ((uint32_t * (*)())0xb0)();
     // GetB0Table()

    b0table[0x17] = kernelPatchDest;
}

static const uint32_t shellReplacement[] = {
    addiu(Reg::R0, Reg::R0, 0),
    j(reinterpret_cast<uintptr_t>(installKernelPatch)),
    nop(),
};

[[noreturn]] void reboot()
{
    psyqo::Kernel::fastEnterCriticalSection();
    constexpr uint32_t breakHandler[] = {
        mtc0(Reg::R0, 7),
        jr(Reg::RA),
        rfe(),
    };

    __builtin_memcpy(reinterpret_cast<void *>(0x40), breakHandler,
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
}

void Hello::prepare()
{
    psyqo::GPU::Configuration config;
    config.set(psyqo::GPU::Resolution::W320)
        .set(psyqo::GPU::VideoMode::AUTO)
        .set(psyqo::GPU::ColorMode::C15BITS)
        .set(psyqo::GPU::Interlace::PROGRESSIVE);
    gpu().initialize(config);

    reboot();
}

void Hello::createScene()
{
    m_font.uploadSystemFont(gpu());
    pushScene(&helloScene);
}

void HelloScene::frame()
{
    if (m_anim == 0)
    {
        m_direction = true;
    }
    else if (m_anim == 255)
    {
        m_direction = false;
    }
    psyqo::Color bg{{.r = 0, .g = 64, .b = 91}};
    bg.r = m_anim;
    hello.gpu().clear(bg);
    if (m_direction)
    {
        m_anim++;
    }
    else
    {
        m_anim--;
    }

    psyqo::Color c = {{.r = 255, .g = 255, .b = uint8_t(255 - m_anim)}};
    hello.m_font.print(hello.gpu(), "Hello World!", {{.x = 16, .y = 32}}, c);
    ramsyscall_printf("0x80080196 = %04X\n", *(uint16_t *)0x80080196);
}

int main() { return hello.run(); }
