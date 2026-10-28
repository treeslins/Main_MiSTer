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
    // Text-only callers may omit the legacy icon table even with a marker.
    page.clear();
    page.set(0,"Marker",true,false,false,0,32,17);
    ZhDrawPage(page,"",8,0,frame,nullptr);
    // Cheat status is a fixed three-byte prefix: space, icon, space.
    // The pixels of this prefix must not move with a long UTF-8 name.
    icons[0x1a][0]=0x55;
    icons[0x1a][3]=0x7f;
    const char *cheat_name=u8" \x1a 中文AB123中文CD456中文EF789中文GH012中文IJ345";
    page.clear();
    page.set(0,cheat_name,true,false,true,0,32,17,0,false,"",3);
    assert(page.rows[0].prefix_width==20);
    uint8_t stationary[16*256];
    ZhDrawPage(page,"",8,0,stationary,icons);
    for(unsigned offset : {1u,7u,20u,65u}) {
        page.rows[0].scroll=offset;
        ZhDrawPage(page,"",8,0,frame,icons);
        for(unsigned x=0;x<20;++x) {
            assert(frame[22+x]==stationary[22+x]);
            assert(frame[256+22+x]==stationary[256+22+x]);
        }
        assert(memcmp(frame+42,stationary+42,214) ||
               memcmp(frame+256+42,stationary+256+42,214));
    }
    // Reusing the row for a normal write removes the old fixed prefix.
    page.set(0,"Normal",false,false);
    assert(page.rows[0].prefix_width==0 && page.rows[0].fixed_prefix.empty());
    // A byte offset inside a Hanzi must not produce invalid UTF-8 fragments.
    page.set(0,u8"中文",false,false,true,0,32,0,0,false,"",2);
    assert(page.rows[0].fixed_prefix.empty() && page.rows[0].text==u8"中文");
    page.set(0,u8"中文",false,false,true,0,32,0,0,false,"",3);
    assert(page.rows[0].fixed_prefix==u8"中" && page.rows[0].text==u8"文");
    assert(page.rows[0].prefix_width==12 && page.rows[0].pixel_width==12);
    puts("ZH OSD special offset/background and 32-row bounds: PASS");
}
