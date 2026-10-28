#include "cn/osd_rows.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main()
{
    uint8_t data[32*256] = {};
    uint8_t reference[16*256];
    memset(data,0x5a,16*256);
    memcpy(reference,data,sizeof(reference));
    memset(data+16*256,0xab,3*256);
    data[19*256]=0xcd;
    int dirty=0;
    ZhClearUnusedOsdRows(data,&dirty,19);
    assert(dirty == ((1<<16)|(1<<17)|(1<<18)));
    for(int row=16; row<19; ++row)
        for(int col=0; col<256; ++col) assert(data[row*256+col]==0);
    assert(memcmp(data,reference,sizeof(reference))==0);
    assert(data[19*256]==0xcd);
    // Menu-core rows 16,17,18 contain the special top status bar.
    // Their contents and dirty bits must survive a Chinese 16px update.
    uint8_t header[32*256] = {};
    memset(header+16*256,0x10,256);
    memset(header+17*256,0x55,256); // MiSTer/date/SD/BT status glyphs
    memset(header+18*256,0x18,256);
    int header_dirty=(1<<16)|(1<<17)|(1<<18);
    ZhClearUnusedOsdRows(header,&header_dirty,19,true);
    assert(header_dirty==((1<<16)|(1<<17)|(1<<18)));
    for(int x=0;x<256;++x) {
        assert(header[16*256+x]==0x10);
        assert(header[17*256+x]==0x55);
        assert(header[18*256+x]==0x18);
    }
    // No redundant dirty rows when nothing changed.
    dirty=0;
    ZhClearUnusedOsdRows(data,&dirty,19);
    assert(dirty==0);
    // On a standard 16-row core, the extra rows must never be marked.
    memset(data+16*256,0xff,3*256);
    ZhClearUnusedOsdRows(data,&dirty,16);
    assert(dirty==0);
    assert(data[16*256]==0xff);
    puts("Chinese 16/19 physical OSD row clean/dirty bounds: PASS");
}
