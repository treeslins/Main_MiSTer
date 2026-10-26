// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
#include <string.h>

// Chinese text uses 16 physical rows (eight 16-pixel menu entries).
// The menu FPGA interface can request 19 rows. Always clear rows 16..18
// before uploading them, never leak stale data from a previous display.
inline void ZhClearUnusedOsdRows(uint8_t *osdbuf, int *osdset, int physical_rows)
{
    if(!osdbuf || !osdset) return;
    if(physical_rows>19) physical_rows=19;
    for(int row=16; row<physical_rows; ++row) {
        uint8_t *line=osdbuf + row*256;
        bool stale=false;
        for(int col=0; col<256; ++col) {
            if(line[col]) { stale=true; break; }
        }
        if(stale) {
            memset(line,0,256);
            *osdset |= 1<<row;
        }
    }
}
