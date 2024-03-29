#ifndef MONO_FONTS_12_H
#define MONO_FONTS_12_H
#include "pico/platform.h"
#include "text_mode_font.h"

#define MONO_FONT_HEIGHT 12
#define MONO_FONT_WIDTH 8
#define MONO_FONT_GLYPH_COUNT 256
/** Number of different 256-glyph font blocks */
#define MONO_FONTS_COUNT 4
#define MONO_FONT_REGULAR 0
#define MONO_FONT_REGULAR_OFFSET 0
#define MONO_FONT_REGULAR_GLYPH(x) (x)
#define MONO_FONT_BOLD 1
#define MONO_FONT_BOLD_OFFSET 256
#define MONO_FONT_BOLD_GLYPH(x) ((x) + MONO_FONT_BOLD_OFFSET)
#define MONO_FONT_LIGHT_GRAPHICS 2
#define MONO_FONT_LIGHT_GRAPHICS_OFFSET 512
#define MONO_FONT_LIGHT_GRAPHICS_GLYPH(x) ((x) + MONO_FONT_LIGHT_GRAPHICS_OFFSET)
#define MONO_FONT_DARK_GRAPHICS 3
#define MONO_FONT_DARK_GRAPHICS_OFFSET 768
#define MONO_FONT_DARK_GRAPHICS_GLYPH(x) ((x) + MONO_FONT_DARK_GRAPHICS_OFFSET)

#define MONO_FONT_12_SECTION_ATTRIBUTE __not_in_flash("monofonts12normal")
/**
 * Raw glyph bitmap data array.
 */
extern const MONO_FONT_12_SECTION_ATTRIBUTE TEXT_MODE_FONT_DATA_TYPE mono_font_12_bitmaps_normal[MONO_FONTS_COUNT][MONO_FONT_GLYPH_COUNT][MONO_FONT_HEIGHT];
/**
 * 12-pixel-high monospaced font descriptor for text_mode rendering routine
 */
extern const MONO_FONT_12_SECTION_ATTRIBUTE text_mode_font mono_font_12_normal;


// UI elements
/** UI radio button unchecked */
#define RADIO_EMPTY (MONO_FONT_LIGHT_GRAPHICS_OFFSET + 0xA0)
/** UI radio button checked */
#define RADIO_FILLED (RADIO_EMPTY + 1)
/** UI radio button with a dot inside for indeterminate state */
#define RADIO_DOTTED (RADIO_EMPTY + 2)
/** UI radio button with an X through it */
#define RADIO_CROSSED (RADIO_EMPTY + 3)
/** UI radio button grayed-out */
#define RADIO_GRAYED (RADIO_EMPTY + 4)
/** UI checkbox unchecked */
#define CHECKBOX_EMPTY (MONO_FONT_DARK_GRAPHICS_OFFSET + 0xA0)
/** UI checkbox checked */
#define CHECKBOX_CHECKED (CHECKBOX_EMPTY + 1)
/** UI checkbox checked, but with a lighter check */
#define CHECKBOX_CHECKED_LIGHT (RADIO_EMPTY + 5)
/** UI checkbox with a dot in it for indeterminate state */
#define CHECKBOX_DOTTED (CHECKBOX_EMPTY + 2)
/** UI checkbox with an X through it */
#define CHECKBOX_CROSSED (CHECKBOX_EMPTY + 3)
/** UI checkbox grayed-out */
#define CHECKBOX_GRAYED (CHECKBOX_EMPTY + 4)
/** UI checkbox fully-filled */
#define CHECKBOX_FILLED (CHECKBOX_EMPTY + 5)
// Various bullets
/** A large circle bullet */
#define BULLET_CIRCLE_LARGE_LIGHT MONO_FONT_LIGHT_GRAPHICS_GLYPH(0xA6)
/** A large circle with a thicker line */
#define BULLET_CIRCLE_LARGE_DARK MONO_FONT_DARK_GRAPHICS_GLYPH(0xA6)
/** A small dot bullet */
#define BULLET_DOT MONO_FONT_LIGHT_GRAPHICS_GLYPH(0xA7)
/** A bigger dot bullet */
#define BULLET_BIG_DOT (BULLET_DOT + 1)
/** An unfilled circular bullet */
#define BULLET_CIRCLE (BULLET_DOT + 2)
/** A medium square filled bullet */
#define BULLET_SQUARE MONO_FONT_DARK_GRAPHICS_GLYPH(0xA7)
/** A big square filled bullet */
#define BULLET_BIG_SQUARE (BULLET_SQUARE + 1)
/** A big square unfilled bullet */
#define BULLET_BOX (BULLET_BIG_SQUARE + 1)
/** A giant square filled bullet */
#define BULLET_GIANT_SQUARE MONO_FONT_DARK_GRAPHICS_GLYPH(0xAA)
/** A giant square unfilled bullet */
#define BULLET_GIANT_BOX MONO_FONT_LIGHT_GRAPHICS_GLYPH(0xAA)
// 
#define CH_DINGBAT_X 0xAB
#define CH_DINGBAT_O 0xAC
#define CH_DINGBAT_CHECK 0xAD
#define CH_DINGBAT_LINE 0xAE
#define CH_DINGBAT_CHECKED_LINE 0xAF
#define CH_DINGBAT_LEFT_HALF_FILLED_CIRCLE 0x9B
#define CH_DINGBAT_RIGHT_HALF_FILLED_CIRCLE 0x9C
#define CH_DINGBAT_FILLED_CIRCLE 0x9D
#define CH_DINGBAT_SINGLE_NOTE 0x9E
#define CH_DINGBAT_TWO_NOTES (CH_DINGBAT_SINGLE_NOTE + 1)
#define CH_DINGBAT_SUIT_HEART 0xF0
#define CH_DINGBAT_SUIT_DIAMOND (CH_DINGBAT_SUIT_HEART + 1)
#define CH_DINGBAT_SUIT_CLUB (CH_DINGBAT_SUIT_HEART + 2)
#define CH_DINGBAT_SUIT_SPADE (CH_DINGBAT_SUIT_HEART + 3)
#define CH_DINGBAT_BRIGHTNESS 0xF4
#define CH_DINGBAT_ARROW_RIGHT 0xE4
#define CH_DINGBAT_ARROW_LEFT (CH_DINGBAT_ARROW_RIGHT + 1)
#define CH_DINGBAT_ARROW_LEFT_RIGHT (CH_DINGBAT_ARROW_RIGHT + 2)
#define CH_DINGBAT_ARROW_DOWN (CH_DINGBAT_ARROW_RIGHT + 3)
#define CH_DINGBAT_ARROW_UP (CH_DINGBAT_ARROW_RIGHT + 4)
#define CH_DINGBAT_ARROW_DOWN_UP (CH_DINGBAT_ARROW_RIGHT + 5)
#define CH_DIGNBAT_UPPER_INTEGRAL 0xEE
#define CH_DIGNBAT_LOWER_INTEGRAL (CH_DIGNBAT_UPPER_INTEGRAL + 1)
//
#define REGULAR_X MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_X)
#define REGULAR_O MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_O)
#define REGULAR_CHECK MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_CHECK)
#define REGULAR_LINE MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_LINE)
#define REGULAR_CHECKED_LINE MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_CHECKED_LINE)
#define REGULAR_LEFT_HALF_FILLED_CIRCLE MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_LEFT_HALF_FILLED_CIRCLE)
#define REGULAR_RIGHT_HALF_FILLED_CIRCLE MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_RIGHT_HALF_FILLED_CIRCLE)
#define REGULAR_FILLED_CIRCLE MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_FILLED_CIRCLE)
#define REGULAR_SINGLE_NOTE MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_SINGLE_NOTE)
#define REGULAR_TWO_NOTES MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_TWO_NOTES)
#define REGULAR_SUIT_HEART MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_SUIT_HEART)
#define REGULAR_SUIT_DIAMOND MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_SUIT_DIAMOND)
#define REGULAR_SUIT_CLUB MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_SUIT_CLUB)
#define REGULAR_SUIT_SPADE MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_SUIT_SPADE)
#define REGULAR_BRIGHTNESS MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_BRIGHTNESS)
#define REGULAR_ARROW_RIGH MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_ARROW_RIGHT)
#define REGULAR_ARROW_LEFT MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_ARROW_LEFT)
#define REGULAR_ARROW_LEFT_RIGHT MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_ARROW_LEFT_RIGHT)
#define REGULAR_ARROW_DOWN MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_ARROW_DOWN)
#define REGULAR_ARROW_UP MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_ARROW_UP)
#define REGULAR_ARROW_DOWN_UP MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_ARROW_DOWN_UP)
//
#define BOLD_X MONO_FONT_DARK_GRAPHICS_GLYPH(CH_DINGBAT_X)
#define BOLD_O MONO_FONT_DARK_GRAPHICS_GLYPH(CH_DINGBAT_O)
#define BOLD_CHECK MONO_FONT_DARK_GRAPHICS_GLYPH(CH_DINGBAT_CHECK)
#define BOLD_LINE MONO_FONT_DARK_GRAPHICS_GLYPH(CH_DINGBAT_LINE)
#define BOLD_CHECKED_LINE MONO_FONT_DARK_GRAPHICS_GLYPH(CH_DINGBAT_CHECKED_LINE)
#define BOLD_LEFT_HALF_FILLED_CIRCLE MONO_FONT_DARK_GRAPHICS_GLYPH(CH_DINGBAT_LEFT_HALF_FILLED_CIRCLE)
#define BOLD_RIGHT_HALF_FILLED_CIRCLE MONO_FONT_DARK_GRAPHICS_GLYPH(CH_DINGBAT_RIGHT_HALF_FILLED_CIRCLE)
#define BOLD_FILLED_CIRCLE MONO_FONT_DARK_GRAPHICS_GLYPH(CH_DINGBAT_FILLED_CIRCLE)
#define BOLD_SINGLE_NOTE MONO_FONT_DARK_GRAPHICS_GLYPH(CH_DINGBAT_SINGLE_NOTE)
#define BOLD_TWO_NOTES MONO_FONT_DARK_GRAPHICS_GLYPH(CH_DINGBAT_TWO_NOTES)
#define BOLD_SUIT_HEART MONO_FONT_DARK_GRAPHICS_GLYPH(CH_DINGBAT_SUIT_HEART)
#define BOLD_SUIT_DIAMOND MONO_FONT_DARK_GRAPHICS_GLYPH(CH_DINGBAT_SUIT_DIAMOND)
#define BOLD_SUIT_CLUB MONO_FONT_DARK_GRAPHICS_GLYPH(CH_DINGBAT_SUIT_CLUB)
#define BOLD_SUIT_SPADE MONO_FONT_DARK_GRAPHICS_GLYPH(CH_DINGBAT_SUIT_SPADE)
#define BOLD_BRIGHTNESS MONO_FONT_DARK_GRAPHICS_GLYPH(CH_DINGBAT_BRIGHTNESS)
#define BOLD_ARROW_RIGH MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_ARROW_RIGHT)
#define BOLD_ARROW_LEFT MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_ARROW_LEFT)
#define BOLD_ARROW_LEFT_RIGHT MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_ARROW_LEFT_RIGHT)
#define BOLD_ARROW_DOWN MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_ARROW_DOWN)
#define BOLD_ARROW_UP MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_ARROW_UP)
#define BOLD_ARROW_DOWN_UP MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_DINGBAT_ARROW_DOWN_UP)

/** Set of increasing bar graph glyphs, useful for small in-line bar graphs. */
#define CH_BAR_GRAPH 0x80
/** Inline bar graph characters from zero to ten */
#define BAR_GRAPH_BASE MONO_FONT_DARK_GRAPHICS_GLYPH(CH_BAR_GRAPH)
/** Inline bar graph character, 0 <= x <= 10 */
#define BAR_GRAPH(x) MONO_FONT_DARK_GRAPHICS_GLYPH(CH_BAR_GRAPH + (x))
/** Inline shaded bar graph characters from zero to ten */
#define BAR_GRAPH_SHADED_BASE MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_BAR_GRAPH)
/** Inline shaded bar graph character, 0 <= x <= 10 */
#define BAR_GRAPH_SHADED(x) MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_BAR_GRAPH + (x))

#define CH_BOX_DRAWING_BLANK 0xE3
#define CH_BOX_DRAWING_LIGHT_SHADE 0xB0
#define CH_BOX_DRAWING_MEDIUM_SHADE (CH_BOX_DRAWING_LIGHT_SHADE + 1)
#define CH_BOX_DRAWING_DARK_SHADE (CH_BOX_DRAWING_LIGHT_SHADE + 2)
#define CH_BOX_DRAWING_FILLED (CH_BOX_DRAWING_LIGHT_SHADE + 3)
#define CH_BOX_DRAWING_ARROW_RIGHT 0xEA
#define CH_BOX_DRAWING_ARROW_LEFT (CH_BOX_DRAWING_ARROW_RIGHT + 1)
#define CH_BOX_DRAWING_ARROW_DOWN (CH_BOX_DRAWING_ARROW_RIGHT + 2)
#define CH_BOX_DRAWING_ARROW_UP (CH_BOX_DRAWING_ARROW_RIGHT + 3)
/** Add this offset to other glyphs in the graphics block to obtain a lighter shaded variant. */
#define GRAPHICS_SHADED 128
/** Provides 2 wide by 3 high bitmaps. */
#define GRAPHICS_2X3 MONO_FONT_DARK_GRAPHICS_OFFSET
/** Either 100 % fill or 50 % fill. */
//#define GRAPHICS_FILLED_BLOCK ((MONO_FONT_GRAPHICS << 8) + 63)
/** Provides 2 wide by 2 high bitmaps. */
#define GRAPHICS_2X2 (MONO_FONT_DARK_GRAPHICS_OFFSET + 64)
#define GRAPHICS_HORIZONTAL_LINES_LEFT ((MONO_FONT_GRAPHICS << 8) + 80)
#define GRAPHICS_HORIZONTAL_LINES_RIGHT (GRAPHICS_HORIZONTAL_LINES_LEFT + 8)
#define GRAPHICS_VERTICAL_LINES_TOP ((MONO_FONT_GRAPHICS << 8) + 96)
#define GRAPHICS_VERTICAL_LINES_BOTTOM (GRAPHICS_VERTICAL_LINES_TOP + 12)
/** Either 75 % fill or 25 % fill. */
#define CH_GRAPHICS_SHADED_BLOCK 0x79

// Scroll bar stuff
#define CH_SCROLL_BAR_ARROW_RIGHT 0x8B
#define CH_SCROLL_BAR_ARROW_LEFT (CH_SCROLL_BAR_ARROW_RIGHT + 1)
#define CH_SCROLL_BAR_ARROW_UP (CH_SCROLL_BAR_ARROW_RIGHT + 2)
#define CH_SCROLL_BAR_ARROW_DOWN (CH_SCROLL_BAR_ARROW_RIGHT + 3)
#define CH_SCROLL_BAR_ARROW_UP_DOWN (CH_SCROLL_BAR_ARROW_RIGHT + 4)
#define CH_SCROLL_BAR_HORIZONTAL_LINE 0x7A
#define CH_SCROLL_BAR_DOWN_RIGHT (CH_SCROLL_BAR_HORIZONTAL_LINE + 1)
#define CH_SCROLL_BAR_UP_RIGHT (CH_SCROLL_BAR_HORIZONTAL_LINE + 2)
#define CH_SCROLL_BAR_VERTICAL (CH_SCROLL_BAR_HORIZONTAL_LINE + 3)
#define CH_SCROLL_BAR_DOWN_LEFT (CH_SCROLL_BAR_HORIZONTAL_LINE + 4)
#define CH_SCROLL_BAR_UP_LEFT (CH_SCROLL_BAR_HORIZONTAL_LINE + 5)
#define SCROLL_BAR_ARROW_RIGHT MONO_FONT_DARK_GRAPHICS_GLYPH(CH_SCROLL_BAR_ARROW_RIGHT)
#define SCROLL_BAR_ARROW_LEFT MONO_FONT_DARK_GRAPHICS_GLYPH(CH_SCROLL_BAR_ARROW_LEFT)
#define SCROLL_BAR_ARROW_UP MONO_FONT_DARK_GRAPHICS_GLYPH(CH_SCROLL_BAR_ARROW_UP)
#define SCROLL_BAR_ARROW_DOWN MONO_FONT_DARK_GRAPHICS_GLYPH(CH_SCROLL_BAR_ARROW_DOWN)
#define SCROLL_BAR_ARROW_UP_DOWN MONO_FONT_DARK_GRAPHICS_GLYPH(CH_SCROLL_BAR_ARROW_UP_DOWN)
#define SCROLL_BAR_HORIZONTAL_LINE MONO_FONT_DARK_GRAPHICS_GLYPH(CH_SCROLL_BAR_HORIZONTAL_LINE)
#define SCROLL_BAR_DOWN_RIGHT MONO_FONT_DARK_GRAPHICS_GLYPH(CH_SCROLL_BAR_DOWN_RIGHT)
#define SCROLL_BAR_UP_RIGHT MONO_FONT_DARK_GRAPHICS_GLYPH(CH_SCROLL_BAR_UP_RIGHT)
#define SCROLL_BAR_VERTICAL MONO_FONT_DARK_GRAPHICS_GLYPH(CH_SCROLL_BAR_VERTICAL)
#define SCROLL_BAR_DOWN_LEFT MONO_FONT_DARK_GRAPHICS_GLYPH(CH_SCROLL_BAR_DOWN_LEFT)
#define SCROLL_BAR_DOWN_RIGHT MONO_FONT_DARK_GRAPHICS_GLYPH(CH_SCROLL_BAR_DOWN_RIGHT)
#define SCROLL_BAR_SHADED_ARROW_RIGHT MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_SCROLL_BAR_ARROW_RIGHT)
#define SCROLL_BAR_SHADED_ARROW_LEFT MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_SCROLL_BAR_ARROW_LEFT)
#define SCROLL_BAR_SHADED_ARROW_UP MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_SCROLL_BAR_ARROW_UP)
#define SCROLL_BAR_SHADED_ARROW_DOWN MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_SCROLL_BAR_ARROW_DOWN)
#define SCROLL_BAR_SHADED_ARROW_UP_DOWN MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_SCROLL_BAR_ARROW_UP_DOWN)
#define SCROLL_BAR_SHADED_HORIZONTAL_LINE MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_SCROLL_BAR_HORIZONTAL_LINE)
#define SCROLL_BAR_SHADED_DOWN_RIGHT MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_SCROLL_BAR_DOWN_RIGHT)
#define SCROLL_BAR_SHADED_UP_RIGHT MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_SCROLL_BAR_UP_RIGHT)
#define SCROLL_BAR_SHADED_VERTICAL MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_SCROLL_BAR_VERTICAL)
#define SCROLL_BAR_SHADED_DOWN_LEFT MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_SCROLL_BAR_DOWN_LEFT)
#define SCROLL_BAR_SHADED_DOWN_RIGHT MONO_FONT_LIGHT_GRAPHICS_GLYPH(CH_SCROLL_BAR_DOWN_RIGHT)

/** Really thick lines that line up with the block arrows. */
#define GRAPHICS_LINE_HORIZONTAL MONO_FONT_DARK_GRAPHICS_GLYPH(0x7A)
#define GRAPHICS_LINE_RIGHT_DOWN (GRAPHICS_LINE_HORIZONTAL + 1)
#define GRAPHICS_LINE_RIGHT_UP (GRAPHICS_LINE_HORIZONTAL + 2)
#define GRAPHICS_LINE_VERTICAL (GRAPHICS_LINE_HORIZONTAL + 3)
#define GRAPHICS_LINE_LEFT_DOWN (GRAPHICS_LINE_HORIZONTAL + 4)
#define GRAPHICS_LINE_LEFT_UP (GRAPHICS_LINE_HORIZONTAL + 5)

#define CH_CURSOR_BLANK 0xF5
#define CH_CURSOR_LINE (CH_CURSOR_BLANK + 1)
#define CH_CURSOR_UNDERLINE (CH_CURSOR_BLANK + 2)
#define CH_CURSOR_LONGER_UNDERLINE (CH_CURSOR_BLANK + 3)
#define CH_CURSOR_BLOCK (CH_CURSOR_BLANK + 4)
#define CH_CURSOR_SHADED (CH_CURSOR_BLANK + 5)
#define CH_CURSOR_BOX (CH_CURSOR_BLANK + 6)
#define CH_CURSOR_RIGHT (CH_CURSOR_BLANK + 7)
#define CH_CURSOR_LEFT (CH_CURSOR_BLANK + 8)
#define CH_CURSOR_UP (CH_CURSOR_BLANK + 9)
#define CH_CURSOR_DOWN (CH_CURSOR_BLANK + 10)

#define REGULAR_CURSOR_BLANK MONO_FONT_REGULAR_GLYPH(CH_CURSOR_BLANK)
#define REGULAR_CURSOR_LINE MONO_FONT_REGULAR_GLYPH(CH_CURSOR_LINE)
#define REGULAR_CURSOR_UNDERLINE MONO_FONT_REGULAR_GLYPH(CH_CURSOR_UNDERLINE)
#define REGULAR_CURSOR_LONGER_UNDERLINE MONO_FONT_REGULAR_GLYPH(CH_CURSOR_LONGER_UNDERLINE)
#define REGULAR_CURSOR_BLOCK MONO_FONT_REGULAR_GLYPH(CH_CURSOR_BLOCK)
#define REGULAR_CURSOR_SHADED MONO_FONT_REGULAR_GLYPH(CH_CURSOR_SHADED)
#define REGULAR_CURSOR_BOX MONO_FONT_REGULAR_GLYPH(CH_CURSOR_BOX)
#define REGULAR_CURSOR_RIGHT MONO_FONT_REGULAR_GLYPH(CH_CURSOR_RIGHT)
#define REGULAR_CURSOR_LEFT MONO_FONT_REGULAR_GLYPH(CH_CURSOR_LEFT)
#define REGULAR_CURSOR_UP MONO_FONT_REGULAR_GLYPH(CH_CURSOR_UP)
#define REGULAR_CURSOR_DOWN MONO_FONT_REGULAR_GLYPH(CH_CURSOR_DOWN)

#define BOLD_CURSOR_BLANK MONO_FONT_BOLD_GLYPH(CH_CURSOR_BLANK)
#define BOLD_CURSOR_LINE MONO_FONT_BOLD_GLYPH(CH_CURSOR_LINE)
#define BOLD_CURSOR_UNDERLINE MONO_FONT_BOLD_GLYPH(CH_CURSOR_UNDERLINE)
#define BOLD_CURSOR_LONGER_UNDERLINE MONO_FONT_BOLD_GLYPH(CH_CURSOR_LONGER_UNDERLINE)
#define BOLD_CURSOR_BLOCK MONO_FONT_BOLD_GLYPH(CH_CURSOR_BLOCK)
#define BOLD_CURSOR_SHADED MONO_FONT_BOLD_GLYPH(CH_CURSOR_SHADED)
#define BOLD_CURSOR_BOX MONO_FONT_BOLD_GLYPH(CH_CURSOR_BOX)
#define BOLD_CURSOR_RIGHT MONO_FONT_BOLD_GLYPH(CH_CURSOR_RIGHT)
#define BOLD_CURSOR_LEFT MONO_FONT_BOLD_GLYPH(CH_CURSOR_LEFT)
#define BOLD_CURSOR_UP MONO_FONT_BOLD_GLYPH(CH_CURSOR_UP)
#define BOLD_CURSOR_DOWN MONO_FONT_BOLD_GLYPH(CH_CURSOR_DOWN)



#endif /* MONO_FONTS_12_H */
