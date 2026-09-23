// SPDX-License-Identifier: GPL-3.0-or-later
#include "zh_font.h"
extern "C" const unsigned char _binary_localization_zh_font_bin_start[];

static uint32_t read32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}

const ZhGlyph *ZhFindGlyph(uint32_t code)
{
    const unsigned char *data=_binary_localization_zh_font_bin_start;
    unsigned lo=0,hi=read32(data);
    while(lo<hi) {
        unsigned mid=lo+(hi-lo)/2;
        const unsigned char *record=data+4+mid*37;
        uint32_t key=read32(record);
        if(key<code) lo=mid+1;
        else if(key>code) hi=mid;
        else {
            static thread_local ZhGlyph glyph;
            glyph.code=key; glyph.width=record[4];
            for(unsigned i=0;i<16;++i) glyph.columns[i]=record[5+i*2]|(record[6+i*2]<<8);
            return &glyph;
        }
    }
    return nullptr;
}
