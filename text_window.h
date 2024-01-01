#ifndef TEXT_WINDOW_H
#define TEXT_WINDOW_H
#include "text_buffer.h"

/**
 * Allows rendering text to a smaller window within a text_buffer.
 * Notes that this does not buffer the text separately from the text_buffer;
 * it directly renders text into the text_buffer's buffer.
 */
typedef struct text_window
{
    /** Parent text_buffer that actual rendering goes to. */
    text_buffer* parent;
    /** Base coordinates. */
    coord location;
    /** Bounding box size. */
    coord size;
    /** Current cursor location relative to the base location. */
    coord cursor;
    /** Current foreground and background colors for writing text. */
    color_pair colors;
    /** Value used as a blank character. */
    text_glyph blank;
    /** Current font ID for writing text. */
    unsigned char font;
} text_window;

/**
 * Creates a text_window of a given size.
 */
text_window* text_window_ctor(text_buffer* parent, coord location, coord size);

/**
 * Initializes a text_window whose memory has already been fully allocated.
 */
void text_window_ctor_in_place(text_window* self, text_buffer* parent, coord location, coord size);

/**
 * Deallocates a text_window.
 */
static inline void text_window_dtor(text_window* self)
{
    free(self);
}

/**
 * Moves the cursor to a given location.
 * No bounds check is performed.
 */
static inline void text_window_set_cursor(text_window* self, coord_x x, coord_y y)
{
    self->cursor.x = x;
    self->cursor.y = y;
}

/**
 * Set foreground and background colors that will be used for writing.
 */
static inline void text_window_set_colors(text_window* self, text_color foreground, text_color background)
{
    self->colors.foreground = foreground;
    self->colors.background = background;
}

/**
 * Returns a pointer to the cell the cursor currently points to.
 */
static inline text_cell* text_window_cursor(text_window* self)
{
    return text_buffer_cell(self->parent, self->location.x + self->cursor.x, self->location.y + self->cursor.y);
}

/**
 * Returns a pointer to a cell at a given location.
 */
static inline text_cell* text_window_cell(text_window* self, coord_x x, coord_y y)
{
    return text_buffer_cell(self->parent, self->location.x + x, self->location.y + y);
}

/**
 * Returns a pointer to a cell at a given location.
 */
static inline text_cell* text_window_cell2(text_window* self, coord c)
{
    return text_buffer_cell(self->parent, self->location.x + c.x, self->location.y + c.y);
}

/** Moves cursor to start of line. */
static inline void text_window_home(text_window* self)
{
    self->cursor.x = 0;
}

/** Moves cursor to end of line. */
static inline void text_window_end(text_window* self)
{
    self->cursor.x = self->size.x - 1;
}

/** Moves cursor to top of screen without changing column. */
static inline void text_window_top(text_window* self)
{
    self->cursor.y = 0;
}

/** Moves cursor to bottom of screen without changing column. */
static inline void text_window_bottom(text_window* self)
{
    self->cursor.y = self->size.y - 1;
}

/** Moves cursor to the top left corner. */
static inline void text_window_home_up(text_window* self)
{
    self->cursor.y = self->cursor.x = 0;
}

/** Moves cursor to the right, but stops at the right edge. */
static inline void text_window_right(text_window* self)
{
    if (self->cursor.x < self->size.x - 1)
        self->cursor.x++;
}

/** Moves cursor to the left, but stops at the left edge. */
static inline void text_window_left(text_window* self)
{
    if (self->cursor.x > 0)
        self->cursor.x--;
}

/** Moves cursor down one line, without changing the column, stopping at the bottom.  */
static inline void text_window_down(text_window* self)
{
    if (self->cursor.y < self->size.y - 1)
        self->cursor.y++;
}

/** Moves cursor up one line, without changing the column, stopping at the top. */
static inline void text_window_up(text_window* self)
{
    if (self->cursor.y > 0)
        self->cursor.y--;
}

/** Moves cursor to start of next line. */
static inline void text_window_newline_no_scroll(text_window* self)
{
    text_window_home(self);
    text_window_down(self);
}

/** 
 * Erases rest of current line, and moves cursor to start of next line.
 */
void text_window_newline_no_scroll_clear(text_window* self);

/**
 * Scrolls the text_window down some number of lines.
 * The cursor is not changed.
 */
void text_window_scroll_down_lines(text_window* self, unsigned lines);

/** Scrolls down one line. */
static inline void text_window_scroll_down(text_window* self)
{
    text_window_scroll_down_lines(self, 1);
}

/** Moves cursor to start of next line, scrolling if already at the bottom. */
static inline void text_window_newline(text_window* self)
{
    text_window_home(self);
    if (self->cursor.y < self->size.y - 1)
        text_window_down(self);
    else
        text_window_scroll_down(self);
}

/**
 * Erases the rest of the current line, moves cursor to start of next line,
 * and scrolls if already at the bottom.
 */
void text_window_newline_clear(text_window* self);

/**
 * Moves the cursor right one cell.
 * When it reaches the right edge, moves to the start of the next line.
 * When it reaches the bottom right, moves to top right.
 */
static inline void text_window_next_circular(text_window* self)
{
    if (++self->cursor.x >= self->size.x) {
        text_window_home(self);
        if (++self->cursor.y >= self->size.y)
            text_window_top(self);
    }
}

/**
 * Gets a pointer to the current cursor location, and then advances the cursor.
 * @return Pointer to cursor location BEFORE the cursor was advanced.
 */
static inline text_cell* text_window_cursor_next_circular(text_window* self)
{
    text_cell* here = text_window_cursor(self);
    text_window_next_circular(self);
    return here;
}

/**
 * Writes a character to the buffer with the current font and colors and advances the cursor.
 */
static inline void text_window_put_char(text_window* self, char ch)
{
    text_cell* cell = text_window_cursor_next_circular(self);
    cell->char_font.character = ch;
    cell->char_font.font_id = self->font;
    cell->foreground = self->colors.foreground;
    cell->background = self->colors.background;
}

/**
 * Writes a 16-bit font+glyph to the buffer with the current colors and advances the cursor.
 */
static inline void text_window_put_glyph(text_window* self, text_glyph ch)
{
    text_cell* cell = text_window_cursor_next_circular(self);
    cell->glyph = ch;
    cell->foreground = self->colors.foreground;
    cell->background = self->colors.background;
}

/**
 * Writes a string to the buffer with the current font and colors and advances the cursor.
 */
void text_window_put_string(text_window* self, const char* str);

/**
 * Fully writes a string to the buffer with word wrap using the current font and colors and advances the cursor.
 */
void text_window_put_string_word_wrap(text_window* self, const char* str);

/**
 * Writes a string to the buffer with word wrap using the current font and colors and advances the cursor.
 * If the string was fully printed, the cursor is left at the next character cell to print at.
 * If the string does not fit fully, stops when the window is full and returns a pointer to
 * the first character of the next line of text to print, and the cursor is placed at the bottom left.
 * @return If *return == '\0', the string was fully printed.
 */
const char* text_window_put_string_word_wrap_partial(text_window* self, const char* str);

/**
 * Writes a single-line string to the buffer, horizontally centered.
 */
void text_window_put_string_centered_line(text_window* self, const char* str);

/**
 * Writes a character to the buffer without changing font or colors and advances the cursor.
 */
static inline void text_window_overwrite_char(text_window* self, char ch)
{
    text_window_cursor_next_circular(self)->char_font.character = ch;
}

/**
 * Writes a string to the buffer without changing font or colors and advances the cursor.
 */
void text_window_overwrite_string(text_window* self, const char* str);

/**
 * Writes a 16-bit font+glyph to the buffer without changing colors and advances the cursor.
 */
static inline void text_window_overwrite_glyph(text_window* self, text_glyph ch)
{
    text_window_cursor_next_circular(self)->glyph = ch;
}

/** Erases the entire buffer, using current colors. */
void text_window_erase(text_window* self);

#endif /* TEXT_WINDOW_H */
