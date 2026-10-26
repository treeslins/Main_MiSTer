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
