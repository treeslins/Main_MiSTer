#include "zh_font.h"
#include <assert.h>
#include <stdio.h>

// No bundled or generated Unifont. Requires MISTER_CN_FONT_PATH pointing
// to the already generated, pinned SIL OFL 1.1 misterfpga.cn file.
int main() {
    assert(ZhFontReady());
    const ZhGlyph *zh = ZhFindGlyph(0x4E2D); // 中
    assert(zh && zh->width == 12);
    // M16CN003 has MSB-first column bytes; FPGA OSD consumes LSB-first.
    assert(zh->columns[0] == 0x01E0);
    assert(zh->columns[5] == 0x1FFC);
    assert((zh->columns[5] & 0xC003) == 0); // Two blank rows above and below.
    const ZhGlyph *latin = ZhFindGlyph('A');
    assert(latin && latin->width == 6 && latin->columns[0] == 0x1FE0);
    auto bottom = [](uint32_t cp) {
        const ZhGlyph *glyph = ZhFindGlyph(cp);
        assert(glyph);
        uint16_t ink = 0;
        for (unsigned x = 0; x < glyph->width; ++x) ink |= glyph->columns[x];
        for (int y = 15; y >= 0; --y) if (ink & (1u << y)) return y;
        return -1;
    };
    assert(bottom('A') == bottom(0x4E2D));
    assert(bottom('a') == bottom(0x4E2D));
    assert(bottom('0') == bottom(0x4E2D));
    assert(bottom('.') == bottom(0x4E2D));
    assert(bottom('g') == bottom(0x4E2D) + 1); // Descender retains its extra pixel.
    const ZhGlyph *missing = ZhFindGlyph(0x10FFFF);
    assert(!missing);
    puts("Pinned M16CN003: lookup, 6/12 width, mixed-script baseline PASS");
}
