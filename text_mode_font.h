#ifndef TEXT_MODE_FONT_H
#define TEXT_MODE_FONT_H

#if TEXT_MODE_MAX_FONT_WIDTH <= 8
#define TEXT_MODE_FONT_DATA_TYPE unsigned char
#elif TEXT_MODE_MAX_FONT_WIDTH <= 16
#define TEXT_MODE_FONT_DATA_TYPE unsigned short
#else
#define TEXT_MODE_FONT_DATA_TYPE unsigned int
#endif

/**
 * Contains font information used by the text_mode scan line generator routines.
 * The font data supplied shall be formatted as an array of fixed-size glyph bitmaps.
 */
typedef struct text_mode_font
{
    /**
     * Number of pixels drawn per scan.
     * This would be width for a row-major font, or height for a column-major font.
     */
    const unsigned char scan_pixels;
    /**
     * Number of lines per glyph.
     * This would be height for a row-major font, or width for a column-major font.
     */
    const unsigned char scan_lines;
    /**
     * Number of bytes per scan line.
     * 
     * This value is cached for fast indexing.
     */
    const unsigned char bytes_per_scan;
    /**
     * Number of bytes per glyph in the data array.
     * 
     * This value is cached for fast indexing.
     */
    const unsigned char bytes_per_glyph;
    /**
     * Pointer to font data.
     */
    const void* const data;
} text_mode_font;

#endif /* TEXT_MODE_FONT_H */
