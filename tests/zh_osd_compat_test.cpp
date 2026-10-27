#include "zh_render.h"
#include "zh_ui.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main() {
    uint8_t normal_top[16], normal_bottom[16], shifted_top[16], shifted_bottom[16];
    assert(ZhRender(u8"中", normal_top, normal_bottom, 16) == 12);
    assert(ZhRender(u8"中", shifted_top, shifted_bottom, 16,
                    false, false, 0, nullptr, 1) == 12);
    for (int x=0; x<16; ++x) {
        const uint16_t original=(uint16_t)normal_top[x] | ((uint16_t)normal_bottom[x]<<8);
        const uint16_t shifted=(uint16_t)shifted_top[x] | ((uint16_t)shifted_bottom[x]<<8);
        assert(shifted == (uint16_t)(original<<1));
    }
    // CSS-like shift must be applied BEFORE inverse, so the cleared pixels
    // at the top are inverse-white, rather than shifted to black.
    uint8_t inverted_top[16], inverted_bottom[16];
    assert(ZhRender(u8"中", inverted_top, inverted_bottom, 16,
                    true, false, 0, nullptr, 1)==12);
    for (int x=0; x<16; ++x) {
        assert((uint8_t)(inverted_top[x]^shifted_top[x])==0xff);
        assert((uint8_t)(inverted_bottom[x]^shifted_bottom[x])==0xff);
    }

    ZhPage page;
    // Two-line OSD maps physical framebuffer rows 0/1 to logical row 0.
    page.set(0, u8"中", false, false, true, 0, 32, 0, 1, true);
    char background[16][256] = {};
    background[0][0] = 0x01;
    background[1][0] = (char)0x80;
    background[0][15] = 0x40;
    uint8_t frame[16*256];
    memset(frame,0xcc,sizeof(frame));
    unsigned char icons[256][8] = {};
    ZhDrawPage(page, "", 8, 0, frame, icons, background);
    assert(frame[22] == (uint8_t)(shifted_top[0]|0x01));
    assert(frame[256+22] == (uint8_t)(shifted_bottom[0]|0x80));
    assert(frame[22+15] == (uint8_t)(shifted_top[15]|0x40));
    // No additional background in other columns.
    assert(frame[22+1] == shifted_top[1]);
    assert(frame[256+22+1] == shifted_bottom[1]);

    // Clearing the page must remove old items and selection so a second page
    // using the same title does not inherit its previous entries.
    page.set(6,"Old item", true, false);
    assert(page.focus == 6);
    page.clear();
    assert(page.focus == -1 && page.first == 0);
    assert(page.rows[6].source.empty());
    page.set(0,"New item",true,false);
    auto rows=page.visible(8);
    assert(rows.size()==8 && rows[0]==0 && rows[7]==7); // Native blank slots stay in place.
    assert(page.rows[6].source.empty() && page.rows[0].source=="New item");

    // Ensure a 32-row logical page never draws outside 16 physical OSD rows.
    struct Guarded {
       uint8_t front[32];
       uint8_t frame[16*256];
       uint8_t rear[32];
    } guard;
    memset(&guard,0xa5,sizeof(guard));
    ZhPage long_page;
    for (unsigned row=0; row<32; ++row) long_page.set(row,"Page",row==31,false);
    ZhDrawPage(long_page,"Menu",32,0,guard.frame,icons,background);
    for(int i=0;i<32;i++) {
        assert(guard.front[i]==0xa5);
        assert(guard.rear[i]==0xa5);
    }
    puts("ZH OSD special offset/background and 32-row bounds: PASS");
}
