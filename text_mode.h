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
 * Launch this on core 1 to start rendering textual video.
 */
void text_mode_render_loop(void);

/**
 * This is the line generator routine for text mode.
 * @note Call text_mode_setup_interp() on each core that uses this routine.
 * @param write Write pointer
 * @param scanline Scanline number from scanvideo_scanline_number(buffer->scanline_id)
 * @param screen Pointer to text_buffer with page of text to display
 * @param font Pointer to font to use for rendering
 * @return Returns modified write pointer, which you can use to compute the correct length for a COMPOSABLE_RAW_RUN
 */
uint16_t* text_mode_generate_line(uint16_t* write, unsigned scanline, text_buffer* screen, const text_mode_font* font);

/**
 * Sets up the interpolator required by the fast font code.
 */
void text_mode_setup_interp(void);

/**
 * Unclaims the interpolator used by the fast font code.
 */
void text_mode_release_interp(void);

/**
 * Custom video mode for 800x480 TFT LCD.
 */
extern const scanvideo_mode_t tft_mode_480x800_60;

#endif /* SCANVIDEO_TEXT_MODE_H */
