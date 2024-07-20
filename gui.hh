#pragma once

#include <stdint.h>

#include "third_party/nugget/psyqo/font.hh"
#include "third_party/nugget/psyqo/gpu.hh"
#include "third_party/nugget/psyqo/primitives/lines.hh"
#include "third_party/nugget/psyqo/primitives/rectangles.hh"

namespace OpenCheat
{
    class GUI final
    {
    private:
        struct Background
        {
            uint32_t polyLineCmd = 0x48000000;
            psyqo::Vertex polyLinePointA;
            psyqo::Vertex polyLinePointB;
            psyqo::Vertex polyLinePointC;
            psyqo::Vertex polyLinePointD;
            psyqo::Vertex polyLinePointE;
            const uint32_t polyLineEndCmd = 0x50005000;
            psyqo::Prim::TPage darkenAreaTPage;
            psyqo::Prim::Rectangle darkenArea;
        } m_background;

    public:
        void Initialize(psyqo::GPU &gpu);
        void InitFont(psyqo::GPU &gpu);

        void DrawBackground(psyqo::GPU &gpu);
        void DrawList(psyqo::GPU &gpu, int32_t selIndex, int32_t count); // To-do: Add items argument
        void DrawTest(psyqo::GPU &gpu, int32_t selIndex, int32_t count);

        psyqo::Font<> m_font;
    };
}
