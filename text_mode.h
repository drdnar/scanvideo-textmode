#ifndef SCANVIDEO_TEXT_MODE_H
#define SCANVIDEO_TEXT_MODE_H
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "text_buffer.h"
#include "text_window.h"
#include "text_mode_font.h"

/**
 * Pointer to currently active page of text to display.
 * This is latched once per line and can be changed on-demand without any special synchronization.
 * The change will simply take effect on the next scan line.
 */
extern text_buffer* volatile text_mode_current_buffer;

/**
 * Pointer to font to use for rendering.
 * If the font size changes, you probably need to use a different size buffer too.
 * If you need to change both, you should first stop video generation because these are not protected
 * with any synchronization.
 */
extern const text_mode_font* volatile text_mode_current_font;

#if TEXT_MODE_PALETTIZED_COLOR
/**
 * Pointer to currently active palette to use for rendering text.
 * This is latched once per line and can be changed on-demand without any special synchronization.
 * The change will simply take effect on the next scan line.
 */
extern uint16_t* volatile text_mode_current_palette;
#endif

/**
 * Custom video mode for 800x480 TFT LCD.
 */
extern const scanvideo_mode_t tft_mode_480x800_60;

/**
 * Launch this on core 1 to start rendering textual video.
 */
void text_mode_render_loop();

#endif /* SCANVIDEO_TEXT_MODE_H */
