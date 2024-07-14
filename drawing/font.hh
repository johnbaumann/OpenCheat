/*

MIT License

Copyright (c) 2022 PCSX-Redux authors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#pragma once

#include <EASTL/array.h>
#include <EASTL/functional.h>
#include <EASTL/string_view.h>
#include <stdarg.h>
#include <stdint.h>

#include "psyqo/fragments.hh"
#include "psyqo/gpu.hh"
#include "psyqo/primitives/common.hh"
#include "psyqo/primitives/sprites.hh"

namespace OpenCheat {

template <size_t Fragments = 16>
class Font;

class FontBase {
  public:
    virtual ~FontBase() {}
    
    void initialize(psyqo::GPU& gpu, psyqo::Vertex location, psyqo::Vertex glyphSize);
    void unpackFont(psyqo::GPU& gpu, const uint8_t* data, psyqo::Vertex location, psyqo::Vertex textureSize);
    void upload(psyqo::GPU& gpu, psyqo::Vertex location = {{.x = 960, .y = 464}});

    void print(psyqo::GPU& gpu, eastl::string_view text, psyqo::Vertex pos, psyqo::Color color);
    void print(psyqo::GPU& gpu, eastl::string_view text, psyqo::Vertex pos, psyqo::Color color, eastl::function<void()>&& callback,
               psyqo::DMA::DmaCallback dmaCallback);
    void print(psyqo::GPU& gpu, const char* text, psyqo::Vertex pos, psyqo::Color color);
    void print(psyqo::GPU& gpu, const char* text, psyqo::Vertex pos, psyqo::Color color, eastl::function<void()>&& callback,
               psyqo::DMA::DmaCallback dmaCallback);
    void printf(psyqo::GPU& gpu, psyqo::Vertex pos, psyqo::Color color, const char* format, ...) {
        va_list args;
        va_start(args, format);
        vprintf(gpu, pos, color, format, args);
        va_end(args);
    }
    void printf(psyqo::GPU& gpu, psyqo::Vertex pos, psyqo::Color color, eastl::function<void()>&& callback, psyqo::DMA::DmaCallback dmaCallback,
                const char* format, ...) {
        va_list args;
        va_start(args, format);
        vprintf(gpu, pos, color, eastl::move(callback), dmaCallback, format, args);
        va_end(args);
    }
    void vprintf(psyqo::GPU& gpu, psyqo::Vertex pos, psyqo::Color color, const char* format, va_list ap);
    void vprintf(psyqo::GPU& gpu, psyqo::Vertex pos, psyqo::Color color, eastl::function<void()>&& callback, psyqo::DMA::DmaCallback dmaCallback,
                 const char* format, va_list ap);

  protected:
    struct GlyphsFragmentPrologue {
        psyqo::Prim::VRAMUpload upload;
        uint32_t pixel;
        psyqo::Prim::FlushCache flushCache;
        psyqo::Prim::TPage tpage;
    };
    typedef psyqo::Fragments::FixedFragmentWithPrologue<GlyphsFragmentPrologue, psyqo::Prim::Sprite, 48> GlyphsFragment;
    virtual GlyphsFragment& getGlyphFragment(bool increment) = 0;
    virtual void forEach(eastl::function<void(GlyphsFragment&)>&& cb) = 0;

    void innerprint(GlyphsFragment& fragment, psyqo::GPU& gpu, eastl::string_view text, psyqo::Vertex pos, psyqo::Color color);
    void innerprint(GlyphsFragment& fragment, psyqo::GPU& gpu, const char* text, psyqo::Vertex pos, psyqo::Color color);
    void innervprintf(GlyphsFragment& fragment, psyqo::GPU& gpu, psyqo::Vertex pos, psyqo::Color color, const char* format, va_list ap);

  private:
    struct XPrintfInfo;
    GlyphsFragment& printToFragment(psyqo::GPU& gpu, const char* text, psyqo::Vertex pos, psyqo::Color color);
    eastl::array<psyqo::Prim::TexInfo, 224> m_lut;
    psyqo::Vertex m_glyphSize;

    friend struct XPrintfInfo;
};

template <size_t N>
class Font : public FontBase {
  public:
    virtual ~Font() { static_assert(N > 0, "Needs to have at least one fragment"); }

  private:
    virtual GlyphsFragment& getGlyphFragment(bool increment) override {
        auto& fragment = m_fragments[m_index];
        if (increment) {
            if (++m_index == N) {
                m_index = 0;
            }
        }
        return fragment;
    }
    virtual void forEach(eastl::function<void(GlyphsFragment&)>&& cb) override {
        for (auto& fragment : m_fragments) {
            cb(fragment);
        }
    }
    eastl::array<GlyphsFragment, N> m_fragments;
    unsigned m_index = 0;
};

}  // namespace OpenCheat
