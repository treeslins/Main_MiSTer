#include <memory.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include "font.h"


FT_Library library;
FT_Face face;
FTC_Manager ftc_manager;
FTC_CMapCache ftc_mapcache;
FTC_SBitCache ftc_bitcache;
FTC_SBit ftc_sbit;
FTC_ImageTypeRec ftc_imagetype;

unsigned char rendered_font[12];

int utf8_charlen(unsigned char c)
{
    if (c < 0x80)
        return 1;
    if (0xC2 <= c && c < 0xE0)
        return 2;
    if (0xE0 <= c && c < 0xF0)
        return 3;
    if (0xF0 <= c && c < 0xF8)
        return 4;

    return 1; // Invalid UTF8 character but seems to be used for MiSTer special bitmap font
}

int utf8_strlen(const char *s)
{
	int len = 0;
	while (*s)
		len += (*(s++)&0xC0) != 0x80;
	return len;
}

int index_from_utf8(const char *s, int utf8index)
{
	int len = 0;
	int index = 0;

	while (*s)
	{
		if (len == utf8index)
			return index;

		len += (*(s++)&0xC0) != 0x80;
		index++;
	}

	return index;
}

FT_ULong utf8_to_utf32(const unsigned char *c)
{
    FT_ULong utf32;
    int len = utf8_charlen(*c);

    if (len == 1)
    {
        utf32 = (FT_ULong)c;
    }
    else if (len == 2)
    {
        utf32 = ((FT_ULong)(*c++ & 0x1F)) << 6;
        utf32 |= ((FT_ULong)(*c & 0x3F));
    }
    else if (len == 3)
    {
        utf32 = ((FT_ULong)(*c++ & 0x0F)) << 12;
        utf32 |= ((FT_ULong)(*c++ & 0x3F)) << 6;
        utf32 |= ((FT_ULong)(*c & 0x3F));
    }
    else if (len == 4)
    {
        utf32 = ((FT_ULong)(*c++ & 0x07)) << 18;
        utf32 |= ((FT_ULong)(*c++ & 0x3F)) << 12;
        utf32 |= ((FT_ULong)(*c++ & 0x3F)) << 6;
        utf32 |= ((FT_ULong)(*c & 0x3F));
    }
    else
    {
        utf32 = (FT_ULong)0x20;
    }

    return utf32;
}

void freetype_render(const unsigned char *c, bool is_lower_part)
{
    FT_Error error;
    FT_ULong c_utf32;
    FT_UInt glyph_index;

    c_utf32 = utf8_to_utf32(c);
    glyph_index = FTC_CMapCache_Lookup(ftc_mapcache, 0, 0, c_utf32);

    if (!glyph_index)
        printf("FREETYPE2: Glyph not found.\n");

    FTC_Node ftc_node;
    error = FTC_SBitCache_Lookup(ftc_bitcache, &ftc_imagetype, glyph_index, &ftc_sbit, &ftc_node);
    if (error)
         printf("FREETYPE2: Error at FTC_SBitCache_Lookup.\n");

    unsigned char byte;
    int shift_x;
    int shift_y = 5 - ftc_sbit->top + ftc_sbit->height + 2;

  
    memset(rendered_font, 0, 8);
 
    int height_offset = 0;
    if (!is_lower_part)
    {
        height_offset = 5;
        shift_y = 5 - ftc_sbit->top + ftc_sbit->height + 1;
    }
    

    
    for (int row = ftc_sbit->height - height_offset; row >= 0; row--)
    {
        byte = ftc_sbit->buffer[ftc_sbit->pitch * row];
        shift_x = 7;

        for (int i = 0; i < 8; i++)
        {
            if (ftc_sbit->left > i)
            {
                rendered_font[i] = 0;
                continue;
            }

            rendered_font[i] |= ((byte >> shift_x--) & 0x01) << shift_y;
        }

        shift_y--;
    }
}

FT_Error FtcFaceRequester(FTC_FaceID faceID, FT_Library lib, FT_Pointer reqData, FT_Face *face)
{
    (void) faceID;

    FT_Error error;
    error = FT_New_Face(lib, (char *)reqData, 0, face);
    if (error)
        printf("FREETYPE2: Error at FT_New_Face.\n");
    return 0;
}

void freetype_init()
{
    FT_Error error;

    error = FT_Init_FreeType(&library);
    error = FTC_Manager_New(library, 0, 0, 1000000, FtcFaceRequester, (FT_Pointer)"/media/fat/font/MZPXorig.ttf", &ftc_manager);
    if (error)
        printf("FREETYPE2: Error at FTC_Manager_New.\n");

    error = FTC_CMapCache_New(ftc_manager, &ftc_mapcache);
    if (error)
        printf("FREETYPE2: Error at FTC_CMapCache_New.\n");

    error = FTC_SBitCache_New(ftc_manager, &ftc_bitcache);
    if (error)
        printf("FREETYPE2: Error at FTC_SBitCache_New.\n");

    error = FTC_Manager_LookupFace(ftc_manager, 0, &face);
    if (error)
        printf("FREETYPE2: Error at FTC_Manager_LookupFace.\n");


    ftc_imagetype.face_id = 0;
    ftc_imagetype.width = 12;
    ftc_imagetype.height = 12;
    ftc_imagetype.flags = FT_LOAD_MONOCHROME | FT_LOAD_TARGET_MONO;

    printf("FREETYPE2: Initialized.\n");
}