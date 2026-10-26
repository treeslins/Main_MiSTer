#pragma once
#include <stdint.h>

// EXPERIMENTAL, opt-in proof of concept; production menu layout not integrated.
bool mister_cn_enabled();
bool mister_cn_render(unsigned char line, const char *utf8, unsigned char invert,
                      uint8_t *osdbuf, int *dirty_rows);
