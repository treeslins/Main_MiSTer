#include "zh_font.h"
#include <assert.h>
#include <stdio.h>

// No bundled or generated Unifont. Requires MISTER_CN_FONT_PATH pointing
// to the already generated, pinned SIL OFL 1.1 misterfpga.cn file.
int main() {
    assert(ZhFontReady());
    const ZhGlyph *zh = ZhFindGlyph(0x4E2D); // 中
    assert(zh && zh->width == 16);
    // M16CN001 has MSB-first column bytes; FPGA OSD consumes LSB-first.
    assert(zh->columns[0] == 0x03E0);
    assert(zh->columns[7] == 0xFFFE);
    const ZhGlyph *latin = ZhFindGlyph('A');
    assert(latin && latin->width == 8 && latin->columns[0] == 0x3FE0);
    const ZhGlyph *missing = ZhFindGlyph(0x10FFFF);
    assert(!missing);
    puts("Pinned M16CN001: lookup, 8/16 width, vertical bit order PASS");
}
