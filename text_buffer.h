#ifndef TEXT_BUFFER_H
#define TEXT_BUFFER_H

#include "pico.h"
#include "pico/stdlib.h"
#include "hardware/divider.h"
#include <stdlib.h>
#include "coord.h"

/**
 * Type of the colors used in text cells.
 */
typedef unsigned short text_color;

/**
 * Can change to 32 bits if you also need 32 bits per pixel
 * (which would be required for alignment).
 */
typedef unsigned short text_glyph;

/** A foreground/background pair packed as a single item. */
typedef struct color_pair
{
    text_color foreground;
    text_color background;
} color_pair;

/**
 * A single cell in the text buffer.
 * Has font and color information for the cell along with a character code.
 */
typedef struct text_cell
{
    union
    {
        struct {
            /** 8-bit character code */
            char character;
            /** 8-bit font ID code */
            unsigned char font_id;
        } char_font;
        /** Character code and font ID together as a single item.  (Used for pseudographics.)*/
        text_glyph glyph;
    };
    /** Foreground color of cell */
    text_color foreground;
    /** Background color of cell */
    text_color background;
} text_cell;

typedef struct text_buffer
{
    /** Size of the text buffer. */
    coord size;
    /**
     * Current default cursor location. 
     * The default cursor is used for writing routines.
     */
    coord cursor;
    /** Current foreground and background colors for writing text. */
    color_pair colors;
    /** Value used as a blank character. */
    text_glyph blank;
    /** Current font ID for writing text. */
    unsigned char font;
    /** Raw text buffer */
    text_cell buffer[];
} text_buffer;

/**
 * Helper macro to declare a text buffer statically and initialize it properly.
 * @param COLS Width
 * @param ROWS Height
 * @param Default foreground color
 * @param Default background color
 * @param Default blank character code
 * @param Default font ID
 */
#define STATIC_TEXT_BUFFER(COLS, ROWS, FG, BG, BLANK, FONT) \
{ \
    .size = { COLS, ROWS }, \
    .cursor = { 0, 0 }, \
    .colors = { FG, BG }, \
    .blank = BLANK, \
    .font = FONT, \
    .buffer = { \
        [0 ... COLS * ROWS - 1] = { \
            .glyph = BLANK, \
            .foreground = FG, \
            .background = BG \
        } \
    } \
}

/**
 * Creates a text buffer of a given size.
 */
text_buffer* text_buffer_ctor(coord_x cols, coord_y rows);

/**
 * Initializes a text buffer whose memory has already been fully allocated.
 */
void text_buffer_ctor_in_place(text_buffer* self, coord_x cols, coord_y rows);

/**
 * Deallocates a text buffer.
 */
static inline void text_buffer_dtor(text_buffer* self)
{
    free(self);
}

/**
 * Moves the cursor to a given location.
 * No bounds check is performed.
 */
static inline void text_buffer_set_cursor(text_buffer* self, coord_x x, coord_y y)
{
    self->cursor.x = x;
    self->cursor.y = y;
}

/**
 * Set foreground and background colors that will be used for writing.
 */
static inline void text_buffer_set_colors(text_buffer* self, text_color foreground, text_color background)
{
    self->colors.foreground = foreground;
    self->colors.background = background;
}

/**
 * Returns a pointer to the cell the cursor currently points to.
 */
static inline text_cell* text_buffer_cursor(text_buffer* self)
{
    return self->buffer + self->size.x * self->cursor.y + self->cursor.x;
}

/**
 * Returns a pointer to a cell at a given location.
 */
static inline text_cell* text_buffer_cell(text_buffer* self, coord_x x, coord_y y)
{
    return self->buffer + self->size.x * y + x;
}

/**
 * Returns a pointer to a cell at a given location.
 */
static inline text_cell* text_buffer_cell2(text_buffer* self, coord c)
{
    return self->buffer + self->size.x * c.y + c.x;
}

/** Moves cursor to start of line. */
static inline void text_buffer_home(text_buffer* self)
{
    self->cursor.x = 0;
}

/** Moves cursor to end of line. */
static inline void text_buffer_end(text_buffer* self)
{
    self->cursor.x = self->size.x - 1;
}

/** Moves cursor to top of screen without changing column. */
static inline void text_buffer_top(text_buffer* self)
{
    self->cursor.y = 0;
}

/** Moves cursor to bottom of screen without changing column. */
static inline void text_buffer_bottom(text_buffer* self)
{
    self->cursor.y = self->size.y - 1;
}

/** Moves cursor to the top left corner. */
static inline void text_buffer_home_up(text_buffer* self)
{
    self->cursor.y = self->cursor.x = 0;
}

/** Moves cursor to the right, but stops at the right edge. */
static inline void text_buffer_right(text_buffer* self)
{
    if (self->cursor.x < self->size.x - 1)
        self->cursor.x++;
}

/** Moves cursor to the left, but stops at the left edge. */
static inline void text_buffer_left(text_buffer* self)
{
    if (self->cursor.x > 0)
        self->cursor.x--;
}

/** Moves cursor down one line, without changing the column, stopping at the bottom.  */
static inline void text_buffer_down(text_buffer* self)
{
    if (self->cursor.y < self->size.y - 1)
        self->cursor.y++;
}

/** Moves cursor up one line, without changing the column, stopping at the top. */
static inline void text_buffer_up(text_buffer* self)
{
    if (self->cursor.y > 0)
        self->cursor.y--;
}

/** Moves cursor to start of next line. */
static inline void text_buffer_newline_no_scroll(text_buffer* self)
{
    text_buffer_home(self);
    text_buffer_down(self);
}

/**
 * Scrolls the text buffer down some number of lines.
 * The cursor is not changed.
 */
void text_buffer_scroll_down_lines(text_buffer* self, unsigned lines);

/** Scrolls down one line. */
static inline void text_buffer_scroll_down(text_buffer* self)
{
    text_buffer_scroll_down_lines(self, 1);
}

/** Moves cursor to start of next line, scrolling if already at the bottom. */
static inline void text_buffer_newline(text_buffer* self)
{
    text_buffer_home(self);
    if (self->cursor.y < self->size.y - 1)
        text_buffer_down(self);
    else
        text_buffer_scroll_down(self);
}

/**
 * Moves the cursor right one cell.
 * When it reaches the right edge, moves to the start of the next line.
 * When it reaches the bottom right, moves to top right.
 */
static inline void text_buffer_next_circular(text_buffer* self)
{
    if (++self->cursor.x >= self->size.x) {
        text_buffer_home(self);
        if (++self->cursor.y >= self->size.y)
            text_buffer_top(self);
    }
}

/**
 * Gets a pointer to the current cursor location, and then advances the cursor.
 * @return Pointer to cursor location BEFORE the cursor was advanced.
 */
static inline text_cell* text_buffer_cursor_next_circular(text_buffer* self)
{
    text_cell* here = text_buffer_cursor(self);
    text_buffer_next_circular(self);
    return here;
}

/**
 * Writes a character to the buffer with the current font and colors and advances the cursor.
 */
static inline void text_buffer_put_char(text_buffer* self, char ch)
{
    text_cell* cell = text_buffer_cursor_next_circular(self);
    cell->char_font.character = ch;
    cell->char_font.font_id = self->font;
    cell->foreground = self->colors.foreground;
    cell->background = self->colors.background;
}

/**
 * Writes a 16-bit font+glyph to the buffer with the current colors and advances the cursor.
 */
static inline void text_buffer_put_glyph(text_buffer* self, text_glyph ch)
{
    text_cell* cell = text_buffer_cursor_next_circular(self);
    cell->glyph = ch;
    cell->foreground = self->colors.foreground;
    cell->background = self->colors.background;
}

/**
 * Writes a string to the buffer with the current font and colors and advances the cursor.
 */
static inline void text_buffer_put_string(text_buffer* self, const char* str)
{
    /* This could be optimized but . . . eh, whatever. */
    while (*str != '\0')
        text_buffer_put_char(self, *str++);
}

/**
 * Writes a character to the buffer without changing font or colors and advances the cursor.
 */
static inline void text_buffer_overwrite_char(text_buffer* self, char ch)
{
    text_buffer_cursor_next_circular(self)->char_font.character = ch;
}

/**
 * Writes a string to the buffer without changing font or colors and advances the cursor.
 */
static inline void text_buffer_overwrite_string(text_buffer* self, const char* str)
{
    /* This could be optimized but . . . eh, whatever. */
    while (*str != '\0')
        text_buffer_overwrite_char(self, *str++);
}

/**
 * Writes a 16-bit font+glyph to the buffer without changing colors and advances the cursor.
 */
static inline void text_buffer_overwrite_glyph(text_buffer* self, text_glyph ch)
{
    text_buffer_cursor_next_circular(self)->glyph = ch;
}

/** Erases the entire buffer, using current colors. */
static inline void text_buffer_erase(text_buffer* self)
{
    text_cell empty = {
        .glyph = self->blank,
        .foreground = self->colors.foreground,
        .background = self->colors.background
    };
    text_cell* cell = self->buffer;
    for (unsigned i = self->size.x * self->size.y; i > 0; i--)
        *cell++ = empty;
}

#endif /* TEXT_BUFFER_H */
