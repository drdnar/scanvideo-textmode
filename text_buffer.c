#include "text_buffer.h"
#include <string.h>


void text_buffer_ctor_in_place(text_buffer* self, coord_x cols, coord_y rows)
{
    self->size.x = cols;
    self->size.y = rows;
    self->cursor.y = self->cursor.x = 0;
    self->colors.foreground = 0;
    self->colors.background = 1;
    self->blank = ' ';
    self->font = 0;
}


text_buffer* text_buffer_ctor(coord_x cols, coord_y rows)
{
    text_buffer* self = malloc(sizeof(text_buffer) + sizeof(text_cell) * cols * rows);
    if (self)
        text_buffer_ctor_in_place(self, cols, rows);
    return self;
}


void text_buffer_scroll_down_lines(text_buffer* self, unsigned n)
{
    if (!n)
        return;
    if (n >= self->size.y) {
        text_buffer_erase(self);
        return;
    }
    size_t delta = self->size.x * n;
    size_t total_cells = self->size.x * self->size.y;
    memmove(self->buffer, self->buffer + delta, (total_cells - delta) * sizeof(text_cell));
    text_cell blank = {
        .glyph = self->blank,
        .foreground = self->colors.foreground,
        .background = self->colors.background
    };
    text_cell* ptr = text_buffer_cell(self, 0, self->size.y - n);
    for (size_t i = 0; i < delta; i++)
        ptr[i] = blank;
}
