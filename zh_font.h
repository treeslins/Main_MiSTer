// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
bool ZhFontReady(); // true only after the pinned M16CN002 font validates
struct ZhGlyph { uint32_t code; unsigned width; uint16_t columns[16]; };

const ZhGlyph *ZhFindGlyph(uint32_t code);
