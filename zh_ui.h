// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
#include <vector>
#include <stdint.h>

std::string ZhTranslate(const std::string &source);
std::string ZhMixedText(const std::string &source);
unsigned ZhTextWidth(const std::string &text);
std::vector<std::string> ZhWrap(const std::string &text, unsigned width);

struct ZhRow {
    std::string source, text;
    bool invert=false, stipple=false, raw=false;
    int mininv=0, maxinv=32;
    unsigned scroll=0;
    unsigned char arrow=0;
};

// Logical row identities remain unchanged. The physical viewport fits the
// existing FPGA's 256x128 OSD, with a pinned navigation row when present.
struct ZhPage {
    ZhRow rows[32];
    int first=0, focus=-1;
    bool manual=false;
    void clear();
    void set(unsigned row, const char *text, bool invert, bool stipple,
             bool raw=false, int mininv=0, int maxinv=32, unsigned char arrow=0);
    std::vector<int> visible(unsigned logical_rows);
    void page(int direction) { first += direction*6; if(first<0) first=0; manual=true; }
};
void ZhDrawPage(ZhPage &page, const std::string &title, unsigned logical_rows,
                int arrows, uint8_t *frame, const unsigned char (*icons)[8]);
