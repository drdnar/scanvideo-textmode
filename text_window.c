#include "text_window.h"
#include <string.h>


void text_window_ctor_in_place(text_window* self, text_buffer* parent, coord location, coord size)
{
    self->parent = parent;
    self->location = location;
    self->size = size;
    self->cursor.y = self->cursor.x = 0;
    self->colors = parent->colors;
    self->blank = parent->blank;
    self->font = parent->font;
}


text_window* text_window_ctor(text_buffer* parent, coord location, coord size)
{
    text_window* self = malloc(sizeof(text_window));
    if (self)
        text_window_ctor_in_place(self, parent, location, size);
    return self;
}


static void text_window_clear_eol(text_window* self)
{
    text_cell* cell = text_window_cursor(self);
    text_cell empty = {
        .glyph = ' ',
        .foreground = self->colors.foreground,
        .background = self->colors.background
    };
    for (unsigned i = self->size.x - self->cursor.x; i > 0; i--)
        *cell++ = empty;
}


static void text_window_clear_overwrite_eol(text_window* self)
{
    text_cell* cell = text_window_cursor(self);
    text_glyph blank = self->blank;
    for (unsigned i = self->size.x - self->cursor.x; i > 0; i--)
        cell++->glyph = blank;
}


void text_window_newline_no_scroll_clear(text_window* self)
{
    text_window_clear_eol(self);
    text_window_newline_no_scroll(self);
}


void text_window_newline_clear(text_window* self)
{
    text_window_clear_eol(self);
    text_window_newline(self);
}


void text_window_scroll_down_lines(text_window* self, unsigned n)
{
    if (!n)
        return;
    if (n >= self->size.y) {
        text_window_erase(self);
        return;
    }
    text_cell* line = text_window_cell(self, 0, 0);
    ptrdiff_t delta = self->parent->size.x;
    size_t size = sizeof(text_cell) * self->size.x;
    for (unsigned i = 0; i < self->size.y - n; i++, line += delta)
        memcpy(line, line + delta * n, size);
    text_cell empty = {
        .glyph = self->blank,
        .foreground = self->colors.foreground,
        .background = self->colors.background
    };
    for (unsigned i = 0; i < n; i++, line += self->parent->size.x - self->size.x)
        for (unsigned j = 0; j < self->size.x; j++)
            *line++ = empty;
}


void text_window_erase(text_window* self)
{
    text_cell empty = {
        .glyph = self->blank,
        .foreground = self->colors.foreground,
        .background = self->colors.background
    };
    text_cell* cell = text_window_cell(self, 0, 0);
    ptrdiff_t delta = self->parent->size.x - self->size.x;
    for (unsigned row = 0; row < self->size.y; row++, cell += delta)
        for (unsigned col = 0; col < self->size.x; col++)
            *cell++ = empty;
}


void text_window_put_string(text_window* self, const char* str)
{
    /* This could be optimized but . . . eh, whatever. */
    char ch;
    while ((ch = *str++) != '\0') {
        if (ch != '\n') {
            text_window_put_char(self, ch);
            if (self->cursor.x == 0 && self->cursor.y == 0) {
                text_window_scroll_down(self);
                self->cursor.y = self->size.y - 1;
            }
        } else
            text_window_newline_clear(self);
    }
}


void text_window_overwrite_string(text_window* self, const char* str)
{
    /* This could be optimized but . . . eh, whatever. */
    char ch;
    while ((ch = *str++) != '\0') {
        if (ch != '\n') {
            text_window_overwrite_char(self, ch);
            if (self->cursor.x == 0 && self->cursor.y == 0) {
                text_window_scroll_down(self);
                self->cursor.y = self->size.y - 1;
            }
        } else {
            text_window_clear_overwrite_eol(self);
            text_window_newline(self);
        }
    }
}


/* TODO: A better way to organize the word wrap code would be to divide it into a section that
 * computes the length of the next line of text to print, and code that actually prints it.
 * The upshot of this is that the layout code can then be run without actually printing anything.
 * The layout code could then also be used for printing centered, right-justified, and even
 * fully-justified paragraphs of text.
 */

/** Internal state for word wrap. */
typedef struct word_wrap
{
    text_cell* cell;
    const char* str;
} word_wrap;


/**
 * Word wrap internal routine: Finds the length of a single "word".
 * @return Number of characters in word.
 */
static size_t text_window_get_word_length(word_wrap* state)
{
    const char* str = state->str;
    size_t length = 0;
    while (true) {
        switch (*str++) {
            case '\0':
            case '\n':
            case ' ':
                return length;
            default:
                length++;
                break;
        }
    }
}


/**
 * Word wrap internal routine: Discards trailing spaces.
 */
static void text_window_eat_spaces(word_wrap* state)
{
    while (*state->str == ' ')
        state->str++;
}


/**
 * Word wrap internal routine: Prints a run of spaces.
 * @return true if cursor hit right edge (spaces after EOL are consumed)
 */
static bool text_window_put_spaces(text_window* self, word_wrap* state)
{
    while (*state->str == ' ') {
        state->str++;
        if (self->cursor.x < self->size.x) {
            text_cell* cell = state->cell;
            cell->char_font.character = ' ';
            cell->char_font.font_id = self->font;
            cell->foreground = self->colors.foreground;
            cell->background = self->colors.background;
            state->cell++;
            self->cursor.x++;
        } else {
            text_window_eat_spaces(state);
            return true;
        }
    }
    return self->cursor.x >= self->size.x;
}


/**
 * Word wrap internal routine: Prints a single word and all following spaces.
 * If the spaces go off the end of the line, they are eaten.
 * If the cursor reached the right edge, the X value is left at size.x.
 * @return true if cursor hit right edge
 */
static bool text_window_put_word(text_window* self, word_wrap* state)
{
    text_cell* cell;
    while (true) {
        char ch = *state->str;
        switch (ch) {
            case '\0':
            case '\n':
                // TODO: If you want, you could check if a newline happens to line up
                // with the natural end-of-line and skip the newline.
                // Whether that is the correct behavior is really a matter of preference/style.
                return false;
            case ' ':
                return text_window_put_spaces(self, state);
            default:
                cell = state->cell;
                cell->char_font.character = ch;
                cell->char_font.font_id = self->font;
                cell->foreground = self->colors.foreground;
                cell->background = self->colors.background;
                state->cell++;
                self->cursor.x++;
                state->str++;
                if (self->cursor.x >= self->size.x) {
                    text_window_eat_spaces(state);
                    return true;
                }
                break;
        }
    }
}


/**
 * Word wrap internal routine: Prints a single line of text and, if there's more text after the end
 * of the line, fills the rest of the line with spaces.
 */
static void text_window_put_line(text_window* self, word_wrap* state)
{
    if (*state->str == ' ') {
        if (text_window_put_spaces(self, state))
            return;
    }
    while (true) {
        if (*state->str == '\0')
            return;
        else if (*state->str == '\n') {
            text_window_clear_eol(self);
            state->str++;
            return;
        }
        int len = text_window_get_word_length(state);
        if (self->cursor.x + len <= self->size.x || self->cursor.x == 0) {
            if (text_window_put_word(self, state))
                return;
        } else {
            text_window_clear_eol(self);
            return;
        }
    }
}


const char* text_window_put_string_word_wrap_partial(text_window* self, const char* str)
{
    word_wrap state = {
        .cell = text_window_cursor(self),
        .str = str
    };
    while (true) {
        text_window_put_line(self, &state);
        if (*state.str == '\0')
            return state.str;
        if (*state.str == '\n')
            return state.str;
        self->cursor.x = 0;
        if (++self->cursor.y == self->size.y) {
            self->cursor.y--;
            return state.str;
        }
        state.cell = text_window_cursor(self);
    }
}


void text_window_put_string_word_wrap(text_window* self, const char* str)
{
    word_wrap state = {
        .cell = text_window_cursor(self),
        .str = str
    };
    text_cell* start;
    while (true) {
        text_window_put_line(self, &state);
        if (*state.str == '\0')
            return;
        self->cursor.x = 0;
        if (++self->cursor.y == self->size.y) {
            self->cursor.y--;
            start = text_window_cursor(self);
            while (true) {
                state.cell = start;
                text_window_put_line(self, &state);
                if (*state.str == '\0')
                    return;
                text_window_scroll_down(self);
                self->cursor.x = 0;
            }
            return;
        }
        state.cell = text_window_cursor(self);
    }
}


/**
 * Internal routine: Prints up to n characters of a string.
 */
static void text_window_put_string_n(text_window* self, const char* str, size_t n)
{
    /* This could be optimized but . . . eh, whatever. */
    char ch;
    while ((ch = *str++) != '\0' && n --> 0) {
        if (ch != '\n') {
            text_window_put_char(self, ch);
            if (self->cursor.x == 0 && self->cursor.y == 0) {
                text_window_scroll_down(self);
                self->cursor.y = self->size.y - 1;
            }
        } else
            text_window_newline_clear(self);
    }
}


/**
 * Internal routine: Prints a bunch of blanks in a row.
 * Useful for layout that needs to adjust spacing.
 */
static void text_window_clear_run(text_window* self, size_t n)
{
    text_cell* cell = text_window_cursor(self);
    while (n --> 0) {
        cell->glyph = self->blank;
        cell->foreground = self->colors.foreground;
        cell->background = self->colors.background;
        cell++;
        self->cursor.x++;
    }
}


void text_window_put_string_centered_line(text_window* self, const char* str)
{
    size_t len = strlen(str);
    if (len > self->size.x) {
        text_window_put_string_n(self, str, self->size.x);
        return;
    }
    int whitespace = self->size.x - len;
    int lspace = whitespace / 2;
    text_window_clear_run(self, lspace);
    text_window_put_string(self, str);
    text_window_clear_run(self, whitespace - lspace);
}
