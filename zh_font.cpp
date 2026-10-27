// SPDX-License-Identifier: GPL-3.0-or-later
// Pinned ZLabs RoundPix 12px glyphs are separately licensed SIL OFL 1.1.
#include "zh_font.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {
const char *const kDefaultFont = "/media/fat/misterfpga.cn";
std::vector<uint8_t> data;
bool attempted = false;
uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
uint8_t reverse8(uint8_t b) {
    b = (uint8_t)((b >> 4) | (b << 4));
    b = (uint8_t)(((b & 0xcc) >> 2) | ((b & 0x33) << 2));
    return (uint8_t)(((b & 0xaa) >> 1) | ((b & 0x55) << 1));
}
// M16CN002 records: Unicode LE32, width byte, reserved[3], 16 top bytes, 16 bottom bytes.
// The 12px face occupies rows 2-13 in a 16px OSD cell.
// Pixel 0 is bit7 in the font but bit0 in the MiSTer OSD, hence reverse8.
bool load_once() {
    if (attempted) return !data.empty();
    attempted = true;
    const char *path = std::getenv("MISTER_CN_FONT_PATH");
    if (!path || !*path) path = kDefaultFont;
    FILE *file = std::fopen(path, "rb");
    if (!file) {
        std::fprintf(stderr, "MiSTer CN: font missing at %s; English fallback\n", path);
        return false;
    }
    if (std::fseek(file, 0, SEEK_END)) { std::fclose(file); return false; }
    const long length = std::ftell(file);
    if (length < 16 || length > 20 * 1024 * 1024 || std::fseek(file, 0, SEEK_SET)) {
        std::fclose(file); return false;
    }
    std::vector<uint8_t> temp(static_cast<size_t>(length));
    bool ok = std::fread(temp.data(), 1, temp.size(), file) == temp.size();
    std::fclose(file);
    if (!ok || std::memcmp(temp.data(), "M16CN002", 8) ||
        le32(temp.data() + 8) != 1) return false;
    const uint32_t count = le32(temp.data() + 12);
    if (!count || (uint64_t)count * 40u + 16u != temp.size()) return false;
    uint32_t previous = 0;
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t *row = temp.data() + 16 + static_cast<size_t>(i) * 40;
        uint32_t cp = le32(row);
        if (cp <= previous || (row[4] != 6 && row[4] != 12) ||
            row[5] || row[6] || row[7]) return false;
        previous = cp;
    }
    data.swap(temp);
    std::fprintf(stderr, "MiSTer CN: loaded %u ZLabs glyphs from %s\n", count, path);
    return true;
}
} // namespace

bool ZhFontReady() { return load_once(); }

const ZhGlyph *ZhFindGlyph(uint32_t code) {
    if (!load_once()) return nullptr;
    size_t lo = 0, hi = (data.size() - 16) / 40;
    const size_t count = hi;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        const uint8_t *r = data.data() + 16 + mid * 40;
        if (le32(r) < code) lo = mid + 1;
        else hi = mid;
    }
    if (lo == count) return nullptr;
    const uint8_t *r = data.data() + 16 + lo * 40;
    if (le32(r) != code) return nullptr;
    static thread_local ZhGlyph glyph;
    glyph.code = code;
    glyph.width = r[4];
    for (unsigned i = 0; i < 16; ++i) {
        uint16_t top = reverse8(r[8 + i]), bottom = reverse8(r[24 + i]);
        glyph.columns[i] = top | (bottom << 8);
    }
    return &glyph;
}
