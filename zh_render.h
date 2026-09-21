// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "zh_font.h"

// Explicit UTF-8 API: legacy OSD strings also contain single-byte icons.
// Never apply this decoder to those strings implicitly.
inline uint32_t ZhDecode(const char *&s)
{
    const unsigned char first = (unsigned char)*s++;
    if (first < 0x80) return first;
    unsigned count;
    uint32_t cp, minimum;
    if (first >= 0xc2 && first <= 0xdf) { count=1; cp=first&31; minimum=0x80; }
    else if (first >= 0xe0 && first <= 0xef) { count=2; cp=first&15; minimum=0x800; }
    else if (first >= 0xf0 && first <= 0xf4) { count=3; cp=first&7; minimum=0x10000; }
    else return 0xfffd;
    for (unsigned i=0; i<count; ++i) {
        unsigned char next=(unsigned char)*s;
        if ((next&0xc0)!=0x80) return 0xfffd;
        ++s;
        cp=(cp<<6)|(next&63);
    }
    return cp<minimum || cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff) ? 0xfffd : cp;
}

// Writes column-major 16-pixel glyphs; clips only at a complete glyph.
// scroll_pixels is a viewport offset, never a byte offset into UTF-8.
inline unsigned ZhRender(const char *text, uint8_t *top, uint8_t *bottom,
                         unsigned width, bool invert=false, bool stipple=false,
                         unsigned scroll_pixels=0)
{
    memset(top, invert?255:0, width);
    memset(bottom, invert?255:0, width);
    unsigned x=0;
    while (*text) {
        uint32_t cp=ZhDecode(text);
        const ZhGlyph *glyph=0;
        for (const auto &g : zh_glyphs) if (g.code==cp) { glyph=&g; break; }
        unsigned w=glyph?glyph->width:16;
        if (scroll_pixels>=w) { scroll_pixels-=w; continue; }
        unsigned start=scroll_pixels;
        scroll_pixels=0;
        if (w-start>width-x) break;
        for (unsigned c=start; c<w; ++c) {
            uint16_t bits=glyph?glyph->columns[c]:((c==0 || c==15)?0xffff:0x8001);
            if (stipple) bits &= (x&1)?0xaaaa:0x5555;
            top[x]=(uint8_t)bits ^ (invert?255:0);
            bottom[x]=(uint8_t)(bits>>8) ^ (invert?255:0);
            ++x;
        }
    }
    return x;
}
