#ifndef CP437_FONT_H
#define CP437_FONT_H
#include "pico/platform.h"
#include "text_mode_font.h"

#define CP437_SECTION_ATTRIBUTE __not_in_flash("cp437")

/**
 * Raw glyph bitmap data array.
 */
extern const CP437_SECTION_ATTRIBUTE unsigned short cp437_bitmaps[1][256][14];
extern const CP437_SECTION_ATTRIBUTE text_mode_font cp437;

#endif /* CP437_FONT_H */
