// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
#include <string.h>

// Chinese text uses 16 physical rows (eight 16-pixel menu entries).
// In MENU_CORE, physical rows 16..18 are the dedicated status/header bar.
// Menu.cpp explicitly writes these rows (16 blank, 17 status text, 18 blank).
// They MUST NOT be wiped when uploading 19 rows.
// Other 19-row contexts may clear unused trailing lines explicitly.
inline void ZhClearUnusedOsdRows(uint8_t *osdbuf, int *osdset, int physical_rows, bool menu_header=false)
{
    if(menu_header) return; // Protect dedicated top header/status rows.
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
