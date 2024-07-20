#include "gui.hh"

#include "third_party/nugget/psyqo/gpu.hh"
#include "third_party/nugget/psyqo/primitives/rectangles.hh"

#include "font.inc"

extern const uint16_t _binary_font_raw_start[];

static constexpr psyqo::Vertex xy(int16_t x, int16_t y)
{
    psyqo::Vertex v = {{x, y}};

    return v;
}

static constexpr psyqo::Vertex wh(int16_t w, int16_t h)
{
    psyqo::Vertex v = {{w, h}};

    return v;
}

namespace OpenCheat
{

    static const psyqo::Prim::Line line(psyqo::Color c, psyqo::Vertex a, psyqo::Vertex b)
    {
        psyqo::Prim::Line l(c);
        l.pointA = a;
        l.pointB = b;

        return l;
    }

    void GUI::DrawBackground(psyqo::GPU &gpu)
    {
        gpu.sendPrimitive(m_background);
    }

    void GUI::DrawList(psyqo::GPU &gpu, int32_t selIndex, int32_t count)
    {
        static const auto WHITE = psyqo::Color{{.r = 255, .g = 255, .b = 255}};
        static const auto GRAY = psyqo::Color{{.r = 48, .g = 48, .b = 48}};
        static const auto LIGHT_GRAY = psyqo::Color{{.r = 96, .g = 96, .b = 96}};

        // To-do: Add items argument
        for (int32_t i = 0; i < count; i++)
        {
            // To-do: Add items argument
            // DrawItem(gpu, items[i]);
            const auto color = i == selIndex ? WHITE : LIGHT_GRAY;
            m_font.print(gpu, "Hello World!", xy(12, 12 + (8 * i)), color);
        }
    }

    void GUI::DrawTest(psyqo::GPU &gpu, int32_t selIndex, int32_t count)
    {
        static const auto WHITE = psyqo::Color{{.r = 255, .g = 255, .b = 255}};
        static const auto GRAY = psyqo::Color{{.r = 48, .g = 48, .b = 48}};
        static const auto LIGHT_GRAY = psyqo::Color{{.r = 96, .g = 96, .b = 96}};

        // To-do: Add items argument
        for (int32_t i = 0; i < count; i++)
        {
            const auto color = i == selIndex ? WHITE : LIGHT_GRAY;
            m_font.print(gpu, "80080196 000C", xy(12, 12 + (8 * i)), color);
        }
    }

    void GUI::InitFont(psyqo::GPU &gpu)
    {
        // Font init
        constexpr psyqo::Vertex location = xy(960, 464);
        constexpr psyqo::Vertex glyphSize = xy(8, 8);
        constexpr psyqo::Rect region = {.pos = {location}, .size = wh(64, 24)};

        m_font.initialize(gpu, location, glyphSize);
        m_font.unpackFont(gpu, s_compressedTexture, location, wh(256, 24));
    }

    void GUI::Initialize(psyqo::GPU &gpu)
    {
        InitFont(gpu);

        // Box
        psyqo::Color lineColor = {{.r = 139, .g = 173, .b = 255}};
        m_background.polyLineCmd |= lineColor.packed;
        m_background.polyLinePointA = xy(9, 9);
        m_background.polyLinePointB = xy(310, 9);
        m_background.polyLinePointC = xy(310, 230);
        m_background.polyLinePointD = xy(9, 230);
        m_background.polyLinePointE = xy(9, 9);
        m_background.darkenAreaTPage.attr
            .set(psyqo::Prim::TPageAttr::FullBackSubFullFront)
            .enableDisplayArea();
        m_background.darkenArea.setColor({{.r = 32, .g = 32, .b = 32}})
            .setSemiTrans();
        m_background.darkenArea.position = xy(10, 10);
        m_background.darkenArea.size = xy(300, 220);
    }
}