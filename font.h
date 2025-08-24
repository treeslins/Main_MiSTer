#ifndef _FONT_H_INCLUDED
#define _FONT_H_INCLUDED

extern unsigned char rendered_font[12];

void freetype_init();
void freetype_render(const unsigned char *c, bool is_lower_part = true);

int utf8_charlen(unsigned char c);
int utf8_strlen(const char *s);
int index_from_utf8(const char *s, int utf8index);

#endif
