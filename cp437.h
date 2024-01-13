#ifndef CP437_FONT_H
#define CP437_FONT_H
#if TEXT_MODE_MAX_FONT_WIDTH > 8
#include "pico/platform.h"
#include "text_mode_font.h"

#if TEXT_MODE_MAX_FONT_WIDTH <= 16
#define CP437_DATA_TYPE unsigned short
#else
#define CP437_DATA_TYPE unsigned int
#endif

#define CP437_FONT_HEIGHT 14
#define CP437_FONT_WIDTH 9
#define CP437_FONT_GLYPH_COUNT 256
#define CP437_FONTS_COUNT 1

#define CP437_SECTION_ATTRIBUTE __not_in_flash("cp437")

/**
 * Raw glyph bitmap data array.
 */
extern const CP437_SECTION_ATTRIBUTE CP437_DATA_TYPE cp437_bitmaps[CP437_FONTS_COUNT][CP437_FONT_GLYPH_COUNT][CP437_FONT_HEIGHT];
extern const CP437_SECTION_ATTRIBUTE text_mode_font cp437;

#endif /* TEXT_MODE_MAX_FONT_WIDTH > 8 */
#endif /* CP437_FONT_H */
