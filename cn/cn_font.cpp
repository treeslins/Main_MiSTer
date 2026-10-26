#include "cn_font.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

namespace {
const char *const kPath = "/media/fat/misterfpga.cn";
std::vector<uint8_t> records;
bool attempted = false;

uint32_t u32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

bool load_font() {
    if (attempted) return !records.empty();
    attempted = true;
    FILE *fp = fopen(kPath, "rb");
    if (!fp) return false;
    if (fseek(fp, 0, SEEK_END)) { fclose(fp); return false; }
    const long size = ftell(fp);
    if (size < 16 || size > 20 * 1024 * 1024 || fseek(fp, 0, SEEK_SET)) {
        fclose(fp);
        return false;
    }
    std::vector<uint8_t> input((size_t)size);
    const bool ok = fread(input.data(), 1, input.size(), fp) == input.size();
    fclose(fp);
    if (!ok || memcmp(input.data(), "M16CN001", 8) || u32le(input.data() + 8) != 1)
        return false;
    const uint32_t count = u32le(input.data() + 12);
    if (count == 0 || 16ull + (uint64_t)count * 40ull != input.size()) return false;
    uint32_t prev = 0;
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t *row = input.data() + 16 + (size_t)i * 40;
        const uint32_t cp = u32le(row);
        if (cp <= prev || (row[4] != 8 && row[4] != 16)) return false;
        prev = cp;
    }
    records.swap(input);
    return true;
}

const uint8_t *find_glyph(uint32_t cp, uint8_t *advance) {
    size_t low = 0, high = (records.size() - 16) / 40;
    while (low < high) {
        const size_t mid = low + (high - low) / 2;
        const uint8_t *r = records.data() + 16 + mid * 40;
        const uint32_t value = u32le(r);
        if (value < cp) low = mid + 1;
        else high = mid;
    }
    if (low == (records.size() - 16) / 40) return nullptr;
    const uint8_t *r = records.data() + 16 + low * 40;
    if (u32le(r) != cp) return nullptr;
    *advance = r[4];
    return r + 8;
}

bool next_utf8(const char *&p, uint32_t &cp) {
    const uint8_t c = (uint8_t)*p++;
    if (!c) return false;
    if (c < 0x80) { cp = c; return true; }
    unsigned n = 0;
    if (c >= 0xC2 && c <= 0xDF) { cp = c & 31; n = 1; }
    else if (c >= 0xE0 && c <= 0xEF) { cp = c & 15; n = 2; }
    else if (c >= 0xF0 && c <= 0xF4) { cp = c & 7; n = 3; }
    else return false;
    for (unsigned i = 0; i < n; ++i) {
        const uint8_t v = (uint8_t)*p;
        if (!v || (v & 0xC0) != 0x80) return false;
        cp = (cp << 6) | (v & 63);
        ++p;
    }
    return !(cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF) ||
             (n == 1 && cp < 0x80) || (n == 2 && cp < 0x800) ||
             (n == 3 && cp < 0x10000));
}
} // namespace

bool mister_cn_enabled() {
    const char *flag = getenv("MISTER_CN_EXPERIMENTAL");
    return flag && strcmp(flag, "1") == 0;
}

bool mister_cn_render(unsigned char line, const char *utf8, unsigned char invert,
                      uint8_t *osdbuf, int *dirty_rows) {
    if (!mister_cn_enabled() || !utf8 || line >= 16 || !osdbuf || !dirty_rows)
        return false;
    bool has_non_ascii = false;
    for (const uint8_t *p = (const uint8_t *)utf8; *p; ++p) {
        if (*p >= 128) has_non_ascii = true;
        if (*p == '\r' || *p == '\n' || *p == 11 || *p == 12) return false;
    }
    if (!has_non_ascii || !load_font()) return false;
    uint8_t upper[256], lower[256];
    const uint8_t mask = invert ? 0xFF : 0;
    memset(upper, mask, sizeof(upper));
    memset(lower, mask, sizeof(lower));
    unsigned x = 22;  // Experimental approximation of the original OSD border.
    for (const char *p = utf8; *p;) {
        uint32_t cp;
        if (!next_utf8(p, cp)) return false;
        uint8_t width = 8;
        const uint8_t *glyph = find_glyph(cp, &width);
        if (!glyph) glyph = find_glyph('?', &width);
        if (!glyph) return false;
        if (x + width > 256) break;
        for (unsigned col = 0; col < width; ++col) {
            upper[x + col] = glyph[col] ^ mask;
            lower[x + col] = glyph[16 + col] ^ mask;
        }
        x += width;
    }
    memcpy(osdbuf + (size_t)(line * 2) * 256, upper, 256);
    memcpy(osdbuf + (size_t)(line * 2 + 1) * 256, lower, 256);
    const uint32_t bits = (1u << (line * 2)) | (1u << (line * 2 + 1));
    *dirty_rows = (int)((uint32_t)*dirty_rows | bits);
    return true;
}
