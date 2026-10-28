// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
bool ZhFontReady(); // true after an M16CN004 file passes runtime format validation
struct ZhGlyph { uint32_t code; unsigned width; uint16_t columns[16]; };

const ZhGlyph *ZhFindGlyph(uint32_t code);
