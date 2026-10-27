#include "zh_font.h"
#include <assert.h>
#include <stdio.h>

// No bundled or generated Unifont. Requires MISTER_CN_FONT_PATH pointing
// to the already generated, pinned SIL OFL 1.1 misterfpga.cn file.
int main() {
    assert(ZhFontReady());
    const ZhGlyph *zh = ZhFindGlyph(0x4E2D); // 中
    assert(zh && zh->width == 12);
    // M16CN002 has MSB-first column bytes; FPGA OSD consumes LSB-first.
    assert(zh->columns[0] == 0x01E0);
    assert(zh->columns[5] == 0x1FFC);
    assert((zh->columns[5] & 0xC003) == 0); // Two blank rows above and below.
    const ZhGlyph *latin = ZhFindGlyph('A');
    assert(latin && latin->width == 6 && latin->columns[0] == 0x07F8);
    const ZhGlyph *missing = ZhFindGlyph(0x10FFFF);
    assert(!missing);
    puts("Pinned M16CN002: lookup, 6/12 width, centered 16-row cells PASS");
}
