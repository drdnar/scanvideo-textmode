#include <stdio.h>
#include "pico.h"
#include "pico/stdlib.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/divider.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "text_buffer.h"
#include "text_window.h"
#include "monofonts12.h"
#include "cp437.h"


////////////////////////////////////////////////////////////////////////////////
/////// CONFIGURATION //////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Putting defines in CMakeLists.txt results in the whole project being
// recompiled even though these settings only affect this file.
// So these settings just live in this file.

// Implement full 800x480 at 180 MHz instead of 760x480 at 150 MHz.
#define FULL_RES
// Use CP437 font instead
//#define USE_CP437
//#define CORE_1_IRQs

#define HEIGHT 480
#define WIDTH 800
#define VIDEO_MODE tft_mode_480x800_60
/* If you change the screen resolution, you may want to override these because the limits chosen
below are calibrated specifically for my 800x480 TFT.
#define TEXT_ROWS 40
#define TEXT_COLS 100
*/

// Adjust text buffer size to match screen size and what can be rendered fast enough.
#ifdef USE_CP437
    // Row 35 doesn't fit fully but the render routine doesn't know how to handle having blank lines
    // at the bottom.  So just add a 35th line so there's valid data to render even though it
    // gets cut off.
    #ifndef TEXT_ROWS
        // 35
        #define TEXT_ROWS ((HEIGHT + CP437_FONT_HEIGHT - 1) / CP437_FONT_HEIGHT)
    #endif
    #ifndef TEXT_COLS
        #ifdef FULL_RES
            // 88
            #define TEXT_COLS ((WIDTH + CP437_FONT_WIDTH - 1) / CP437_FONT_WIDTH)
        #else /* not FULL_RES */
            #if !TEXT_MODE_PALETTIZED_COLOR
                // 88
                #define TEXT_COLS ((WIDTH + CP437_FONT_WIDTH - 1) / CP437_FONT_WIDTH)
            #else /* TEXT_MODE_PALETTIZED_COLOR */
                #define TEXT_COLS 79
            #endif /* TEXT_MODE_PALETTIZED_COLOR */
        #endif /* FULL_RES */
    #endif
#else /* not USE_CP437 */
    #ifndef TEXT_ROWS
        // 40
        #define TEXT_ROWS ((HEIGHT + MONO_FONT_HEIGHT - 1) / MONO_FONT_HEIGHT)
    #endif
    #ifndef TEXT_COLS
        #ifdef FULL_RES
            // 100
            #define TEXT_COLS ((WIDTH + MONO_FONT_WIDTH - 1) / MONO_FONT_WIDTH)
        #else /* not FULL_RES */
            #if !TEXT_MODE_PALETTIZED_COLOR
                #define TEXT_COLS 96
            #else /* TEXT_MODE_PALETTIZED_COLOR */
                #define TEXT_COLS 85
            #endif /* TEXT_MODE_PALETTIZED_COLOR */
        #endif /* FULL_RES */
    #endif
#endif /* USE_CP437 */

#if TEXT_MODE_PALETTIZED_COLOR
/** Default palette for rendering. */
uint16_t main_palette[] = {
    // Since I wired up my TFT as 6 bits per pixel, just map each entry to its index.
    // You can override this with your own palette here.
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
};
/** Current palette for rendering. */
uint16_t* current_palette = main_palette;
#endif


////////////////////////////////////////////////////////////////////////////////
/////// 800 x 480 TFT LCD CONFIGURATION ////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/** Reverse horizontal scan direction */
#define MIRROR_PIN 18
/** Reverse vertical scan direction */
#define UPSIDE_DOWN_PIN 19
/** LCD reset, active low */
#define RESET_PIN 20
/** Control signal for gating V_GL power rail */
#define VGL_PIN 21
/** Control signal for gating AV_DD power rail */
#define AVDD_PIN 22
/** Control signal for gating V_HL power rail */
#define VGH_PIN 26
/** Controls dithering function I guess, probably can't see effect */
#define DITHERING_PIN 18
/** LCD backlight control pin */
#define PWM_PIN 27
/** Used to measure CPU usage with an oscilloscope. */
#define TIMING_MEASURE_PIN 28

enum COLORS6BPP
{   // For this palette, wire odd bits of each channel together and same for even.
    //                B G R
    BLACK         = 0b000000,
    BLUE          = 0b100000,
    GREEN         = 0b001000,
    CYAN          = 0b101000,
    RED           = 0b000010,
    MAGENTA       = 0b100010,
    BROWN         = 0b001001,
    WHITE         = 0b101010,
    GRAY          = 0b010101,
    LIGHT_BLUE    = 0b111010,
    LIGHT_GREEN   = 0b101110,
    LIGHT_CYAN    = 0b111110,
    LIGHT_RED     = 0b101011,
    LIGHT_MAGENTA = 0b111011,
    YELLOW        = 0b001111,
    BRIGHT_WHITE  = 0b111111,
};

/** Custom timings for my 480x800 TFT. */
const scanvideo_timing_t tft_timing_480x800_60_default = {
    .clock_freq = 30000000,

    .h_active = WIDTH,
    .v_active = HEIGHT,

    .h_front_porch = 155,
    .h_pulse = 32,
    .h_total = 1000,
    .h_sync_polarity = 1,

    .v_front_porch = 22,
    .v_pulse = 2,
    .v_total = 525,
    .v_sync_polarity = 1,

    .enable_clock = 1,
    .clock_polarity = 0,

    .enable_den = 0
};
/** Custom mode for my TFT LCD. */
const scanvideo_mode_t tft_mode_480x800_60 = {
    .default_timing = &tft_timing_480x800_60_default,
    .pio_program = &video_24mhz_composable,
    .width = WIDTH,
    .height = HEIGHT,
    .xscale = 1,
    .yscale = 1,
    .yscale_denominator = 1
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifdef USE_CP437
/** Left double-quote */
#define LDQ "\""
/** Right double-quote */
#define RDQ "\""
/** Apostrophe */
#define APS "'"
/** Em dash */
#define EMD "\304"
#else
/** Left double-quote */
#define LDQ "\223"
/** Right double-quote */
#define RDQ "\224"
/** Apostrophe */
#define APS "\222"
/** Em dash */
#define EMD "\227"
#endif


////////////////////////////////////////////////////////////////////////////////
/////// GLOBAL VARIABLES ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/** Currently active font. */
#ifndef USE_CP437
const text_mode_font* current_font = &mono_font_12_normal;
#else
const text_mode_font* current_font = &cp437;
#endif

/** Main text buffer for rendering. */
text_buffer main_buffer = STATIC_TEXT_BUFFER(TEXT_COLS, TEXT_ROWS, BRIGHT_WHITE, BLACK, ' ', 0);
/** Current text screen to render from and write to. */
text_buffer* current_buffer = &main_buffer;


////////////////////////////////////////////////////////////////////////////////
/////// RENDER ROUTINES ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define CORE_1_FUNC(FUNC_NAME) __scratch_y(#FUNC_NAME) FUNC_NAME

static inline uint16_t* CORE_1_FUNC(generate_line_byte_font)(uint16_t* write, unsigned scanline, text_buffer* screen, const text_mode_font* font)
{
    divmod_result_t r = hw_divider_divmod_u32(scanline, font->scan_lines);
    uint32_t char_row = to_remainder_u32(r);
    uint32_t ignored1, ignored2, ignored3;
    register int rjump_delta asm("r8") = 6 * (8 - font->scan_pixels) + 1; // +1 for Thumb mode
    register int rwrite_inc asm("r9") = font->scan_pixels * 2;
#if TEXT_MODE_PALETTIZED_COLOR
    register uint16_t* rend asm("r10") = write + font->scan_pixels * screen->size.x;
#endif
    register uint32_t row asm("r5") = to_quotient_u32(r);
    register uint16_t* rwrite asm("r0") = write;
    register uint32_t rbytes asm("r1") = font->bytes_per_glyph;
    register text_cell* rread asm("r2") = text_buffer_cell(screen, 0, row);
    register uint8_t* rfont asm("r3") = (uint8_t*)font->data + char_row;
#if !TEXT_MODE_PALETTIZED_COLOR
    register uint32_t rcols asm("r4") = screen->size.x;
    assert(sizeof(text_cell) == 6);
#else
    register uint16_t* rpalette asm("r4") = current_palette;
    assert(sizeof(text_cell) == 4);
#endif
    asm volatile(
        // This ends up being a bit over 4-5 cycles per pixel depending on how often it changes
        // between foreground and background color.  Since Latin scripts are mostly white space,
        // most time will be spent in the reset bit path, where there are no branches to flush
        // the (short two-stage) pipeline, so it'll average closer to 4 cycles per pixel.
        // But some semigraphical glyphs can alternate every pixel, so you still should plan for
        // the worst-case 5 cycles.
        // Register allocations:
        // r0: write pointer
        // r1: bytes per glyph
        // r2: read pointer
        // r3: font pointer
        // r4: column counter or palette pointer
        // r5: bitmap
        // r6: foreground color
        // r7: background color
        // r8: loop entry address
        // r9: write increment
        // r10: write end address in palette mode
#if !TEXT_MODE_PALETTIZED_COLOR
        // Including loop overhead, it averages to about 7 cycles per pixel (worst-case).
#else
        // Including loop overhead, it averages to about 8 cycles per pixel (worst-case).
#endif
        "// Cache loop start address\n"
        "    adr     r6, loop_entry%=\n"
        "    add     %[loopstart], r6\n"
        "// Fetch character and colors\n"
        "    ldrh    %[bitmap], [%[read], #0]\n"
        "    // RP2040's CPU cores have the single-cycle multiplier option so shifting isn't any faster\n"
        "    // and is really only useful if you need to save a register.\n"
        "    mul     %[bitmap], %[glyphsize], %[bitmap]\n"
        "    ldrb    %[bitmap], [%[bitmap], %[font]]\n"
#if !TEXT_MODE_PALETTIZED_COLOR
        "    ldrh    r6, [%[read], #2]\n" // offset of text_cell.foreground
        "    ldrh    r7, [%[read], #4]\n" // offset of text_cell.background
        "    add     %[read], %[read], #6\n" // sizeof(text_cell)
#else
        "    ldrb    r6, [%[read], #2]\n" // offset of text_cell.foreground
        "    ldrb    r7, [%[read], #3]\n" // offset of text_cell.background
        "    add     %[read], %[read], #4\n" // sizeof(text_cell)
        "    lsl     r6, r6, #1\n"
        "    ldrh    r6, [%[palette], r6]\n"
        "    lsl     r7, r7, #1\n"
        "    ldrh    r7, [%[palette], r7]\n"
#endif
        "// Unrolled loop\n"
        "    bx      %[loopstart]\n"
        ".balign 4 // ADR requires 32-bit alignment\n"
        "loop_entry%=:"
#define resetbit(bit) "    lsr     %[bitmap], %[bitmap], #1\n" \
"    bcs     set" #bit "%=\n" \
"reset" #bit "%=:\n" \
"    strh    r7, [%[write], #(" #bit " * 2)]\n"
        resetbit(7)
        resetbit(6)
        resetbit(5)
        resetbit(4)
        resetbit(3)
        resetbit(2)
        resetbit(1)
        resetbit(0)
        "    add     %[write], %[writeinc]\n"
#if !TEXT_MODE_PALETTIZED_COLOR
        "    sub     %[cols], #1\n"
#else
        "    cmp     %[end], %[write]\n"
#endif
        "    beq     done%=\n"
        "// Fetch character and colors\n"
        "    ldrh    %[bitmap], [%[read], #0]\n"
        "    mul     %[bitmap], %[glyphsize], %[bitmap]\n"
        "    ldrb    %[bitmap], [%[bitmap], %[font]]\n"
#if !TEXT_MODE_PALETTIZED_COLOR
        "    ldrh    r6, [%[read], #2]\n" // offset of text_cell.foreground
        "    ldrh    r7, [%[read], #4]\n" // offset of text_cell.background
        "    add     %[read], %[read], #6\n" // sizeof(text_cell)
#else
        "    ldrb    r6, [%[read], #2]\n" // offset of text_cell.foreground
        "    ldrb    r7, [%[read], #3]\n" // offset of text_cell.background
        "    add     %[read], %[read], #4\n" // sizeof(text_cell)
        "    lsl     r6, r6, #1\n"
        "    ldrh    r6, [%[palette], r6]\n"
        "    lsl     r7, r7, #1\n"
        "    ldrh    r7, [%[palette], r7]\n"
#endif
        "    bx      %[loopstart]\n"
        // First iteration here is slightly different because there's no preshifting to be done.
#define sethighbit(bit) "set" #bit "%=:\n" \
"    strh    r6, [%[write], #(" #bit " * 2)]\n"
        sethighbit(7)
#define setbit(bit) "    lsr     %[bitmap], %[bitmap], #1\n" \
"    bcc     reset" #bit "%=\n" \
"set" #bit "%=:\n" \
"    strh    r6, [%[write], #(" #bit " * 2)]\n"
        setbit(6)
        setbit(5)
        setbit(4)
        setbit(3)
        setbit(2)
        setbit(1)
        setbit(0)
        "    add     %[write], %[writeinc]\n"
#if !TEXT_MODE_PALETTIZED_COLOR
        "    sub     %[cols], #1\n"
#else
        "    cmp     %[end], %[write]\n"
#endif
        "    beq     done%=\n"
        "// Fetch character and colors\n"
        "    ldrh    %[bitmap], [%[read], #0]\n"
        "    mul     %[bitmap], %[glyphsize], %[bitmap]\n"
        "    ldrb    %[bitmap], [%[bitmap], %[font]]\n"
#if !TEXT_MODE_PALETTIZED_COLOR
        "    ldrh    r6, [%[read], #2]\n" // offset of text_cell.foreground
        "    ldrh    r7, [%[read], #4]\n" // offset of text_cell.background
        "    add     %[read], %[read], #6\n" // sizeof(text_cell)
#else
        "    ldrb    r6, [%[read], #2]\n" // offset of text_cell.foreground
        "    ldrb    r7, [%[read], #3]\n" // offset of text_cell.background
        "    add     %[read], %[read], #4\n" // sizeof(text_cell)
        "    lsl     r6, r6, #1\n"
        "    ldrh    r6, [%[palette], r6]\n"
        "    lsl     r7, r7, #1\n"
        "    ldrh    r7, [%[palette], r7]\n"
#endif
        "    bx      %[loopstart]\n"
        "done%=:"
     :  [read]     "=r" (ignored1),
        [write]    "=r" (write),
#if !TEXT_MODE_PALETTIZED_COLOR
        [cols]     "=r" (ignored2),
#endif
        [loopstart]"=r" (rjump_delta),
        [bitmap]   "=r" (ignored3)
     : "[write]"        (rwrite),
        [glyphsize]"r"  (rbytes),
       "[read]"         (rread),
        [font]     "r"  (rfont),
       "[loopstart]""r" (rjump_delta),
        [writeinc] "r"  (rwrite_inc),
       "[bitmap]"  "r"  (row),
#if !TEXT_MODE_PALETTIZED_COLOR
       "[cols]"    "r"  (rcols)
#else
        [palette]  "r"  (rpalette),
        [end]      "r"  (rend)
#endif
     : "cc", "memory",
       "r6", "r7"
    );
#undef resetbit
#undef sethighbit
#undef setbit
    return write;
}


static inline uint16_t* CORE_1_FUNC(generate_line_short_font)(uint16_t* write, unsigned scanline, text_buffer* screen, const text_mode_font* font)
{
    divmod_result_t r = hw_divider_divmod_u32(scanline, font->scan_lines);
    uint32_t char_row = to_remainder_u32(r);
    uint32_t ignored1, ignored2, ignored3;
    register int rjump_delta asm("r8") = 6 * (16 - font->scan_pixels) + 1; // +1 for Thumb mode
    register int rwrite_inc asm("r9") = font->scan_pixels * 2;
#if TEXT_MODE_PALETTIZED_COLOR
    register uint16_t* rend asm("r10") = write + font->scan_pixels * screen->size.x;
#endif
    register uint32_t row asm("r5") = to_quotient_u32(r);
    register uint16_t* rwrite asm("r0") = write;
    register uint32_t rbytes asm("r1") = font->bytes_per_glyph;
    register text_cell* rread asm("r2") = text_buffer_cell(screen, 0, row);
    register uint16_t* rfont asm("r3") = (uint16_t*)font->data + char_row;
#if !TEXT_MODE_PALETTIZED_COLOR
    register uint32_t rcols asm("r4") = screen->size.x;
    assert(sizeof(text_cell) == 6);
#else
    register uint16_t* rpalette asm("r4") = current_palette;
    assert(sizeof(text_cell) == 4);
#endif
    asm volatile(
        // This ends up being a bit over 4-5 cycles per pixel depending on how often it changes
        // between foreground and background color.  Since Latin scripts are mostly white space,
        // most time will be spent in the reset bit path, where there are no branches to flush
        // the (short two-stage) pipeline, so it'll average closer to 4 cycles per pixel.
        // But some semigraphical glyphs can alternate every pixel, so you still should plan for
        // the worst-case 5 cycles.
        // Register allocations:
        // r0: write pointer
        // r1: bytes per glyph
        // r2: read pointer
        // r3: font pointer
        // r4: column counter or palette pointer
        // r5: bitmap
        // r6: foreground color
        // r7: background color
        // r8: loop entry address
        // r9: write increment
        // r10: write end address in palette mode
#if !TEXT_MODE_PALETTIZED_COLOR
        // Including loop overhead, it averages to 6-7 cycles per pixel (worst-case) depending on the glyph width.
        // (The wider the glyph, the less impact the loop overhead has.)
#else
        // Including loop overhead, it averages to 7-8 cycles per pixel (worst-case) depending on the glyph width.
        // (The wider the glyph, the less impact the loop overhead has.)
#endif
        "// Cache loop start address\n"
        "    adr     r6, loop_entry%=\n"
        "    add     %[loopstart], r6\n"
        "// Fetch character and colors\n"
        "    ldrh    %[bitmap], [%[read], #0]\n"
        "    // RP2040's CPU cores have the single-cycle multiplier option so shifting isn't any faster\n"
        "    // and is really only useful if you need to save a register.\n"
        "    mul     %[bitmap], %[glyphsize], %[bitmap]\n"
        "    ldrh    %[bitmap], [%[bitmap], %[font]]\n"
#if !TEXT_MODE_PALETTIZED_COLOR
        "    ldrh    r6, [%[read], #2]\n" // offset of text_cell.foreground
        "    ldrh    r7, [%[read], #4]\n" // offset of text_cell.background
        "    add     %[read], %[read], #6\n" // sizeof(text_cell)
#else
        "    ldrb    r6, [%[read], #2]\n" // offset of text_cell.foreground
        "    ldrb    r7, [%[read], #3]\n" // offset of text_cell.background
        "    add     %[read], %[read], #4\n" // sizeof(text_cell)
        "    lsl     r6, r6, #1\n"
        "    ldrh    r6, [%[palette], r6]\n"
        "    lsl     r7, r7, #1\n"
        "    ldrh    r7, [%[palette], r7]\n"
#endif
        "// Unrolled loop\n"
        "    bx      %[loopstart]\n"
        ".balign 4 // ADR requires 32-bit alignment\n"
        "loop_entry%=:"
#define resetbit(bit) "    lsr     %[bitmap], %[bitmap], #1\n" \
"    bcs     set" #bit "%=\n" \
"reset" #bit "%=:\n" \
"    strh    r7, [%[write], #(" #bit " * 2)]\n"
        resetbit(15)
        resetbit(14)
        resetbit(13)
        resetbit(12)
        resetbit(11)
        resetbit(10)
        resetbit(9)
        resetbit(8)
        resetbit(7)
        resetbit(6)
        resetbit(5)
        resetbit(4)
        resetbit(3)
        resetbit(2)
        resetbit(1)
        resetbit(0)
        "    add     %[write], %[writeinc]\n"
#if !TEXT_MODE_PALETTIZED_COLOR
        "    sub     %[cols], #1\n"
#else
        "    cmp     %[end], %[write]\n"
#endif
        "    beq     done%=\n"
        "// Fetch character and colors\n"
        "    ldrh    %[bitmap], [%[read], #0]\n"
        "    mul     %[bitmap], %[glyphsize], %[bitmap]\n"
        "    ldrh    %[bitmap], [%[bitmap], %[font]]\n"
#if !TEXT_MODE_PALETTIZED_COLOR
        "    ldrh    r6, [%[read], #2]\n" // offset of text_cell.foreground
        "    ldrh    r7, [%[read], #4]\n" // offset of text_cell.background
        "    add     %[read], %[read], #6\n" // sizeof(text_cell)
#else
        "    ldrb    r6, [%[read], #2]\n" // offset of text_cell.foreground
        "    ldrb    r7, [%[read], #3]\n" // offset of text_cell.background
        "    add     %[read], %[read], #4\n" // sizeof(text_cell)
        "    lsl     r6, r6, #1\n"
        "    ldrh    r6, [%[palette], r6]\n"
        "    lsl     r7, r7, #1\n"
        "    ldrh    r7, [%[palette], r7]\n"
#endif
        "    bx      %[loopstart]\n"
        // First iteration here is slightly different because there's no preshifting to be done.
#define sethighbit(bit) "set" #bit "%=:\n" \
"    strh    r6, [%[write], #(" #bit " * 2)]\n"
        sethighbit(15)
#define setbit(bit) "    lsr     %[bitmap], %[bitmap], #1\n" \
"    bcc     reset" #bit "%=\n" \
"set" #bit "%=:\n" \
"    strh    r6, [%[write], #(" #bit " * 2)]\n"
        setbit(14)
        setbit(13)
        setbit(12)
        setbit(11)
        setbit(10)
        setbit(9)
        setbit(8)
        setbit(7)
        setbit(6)
        setbit(5)
        setbit(4)
        setbit(3)
        setbit(2)
        setbit(1)
        setbit(0)
        "    add     %[write], %[writeinc]\n"
#if !TEXT_MODE_PALETTIZED_COLOR
        "    sub     %[cols], #1\n"
#else
        "    cmp     %[end], %[write]\n"
#endif
        "    beq     done%=\n"
        "// Fetch character and colors\n"
        "    ldrh    %[bitmap], [%[read], #0]\n"
        "    mul     %[bitmap], %[glyphsize], %[bitmap]\n"
        "    ldrh    %[bitmap], [%[bitmap], %[font]]\n"
#if !TEXT_MODE_PALETTIZED_COLOR
        "    ldrh    r6, [%[read], #2]\n" // offset of text_cell.foreground
        "    ldrh    r7, [%[read], #4]\n" // offset of text_cell.background
        "    add     %[read], %[read], #6\n" // sizeof(text_cell)
#else
        "    ldrb    r6, [%[read], #2]\n" // offset of text_cell.foreground
        "    ldrb    r7, [%[read], #3]\n" // offset of text_cell.background
        "    add     %[read], %[read], #4\n" // sizeof(text_cell)
        "    lsl     r6, r6, #1\n"
        "    ldrh    r6, [%[palette], r6]\n"
        "    lsl     r7, r7, #1\n"
        "    ldrh    r7, [%[palette], r7]\n"
#endif
        "    bx      %[loopstart]\n"
        "done%=:"
    :  [read]     "=r" (ignored1),
       [write]    "=r" (write),
#if !TEXT_MODE_PALETTIZED_COLOR
       [cols]     "=r" (ignored2),
#endif
       [loopstart]"=r" (rjump_delta),
       [bitmap]   "=r" (ignored3)
    : "[write]"        (write),
       [glyphsize]"r"  (rbytes),
      "[read]"         (rread),
       [font]     "r"  (rfont),
      "[loopstart]""r" (rjump_delta),
       [writeinc] "r"  (rwrite_inc),
      "[bitmap]"  "r"  (row),
#if !TEXT_MODE_PALETTIZED_COLOR
      "[cols]"    "r"  (rcols)
#else
       [palette]  "r"  (rpalette),
       [end]      "r"  (rend)
#endif
    : "cc", "memory",
      "r6", "r7"
    );
#undef resetbit
#undef sethighbit
#undef setbit
    return write;
}


void CORE_1_FUNC(run_video)()
{
#ifdef CORE_1_IRQs
    scanvideo_setup(&VIDEO_MODE);
    scanvideo_timing_enable(true);
#endif
    while (true) {
        struct scanvideo_scanline_buffer* buffer = scanvideo_begin_scanline_generation(true);
        gpio_put(TIMING_MEASURE_PIN, 1);
        uint16_t* write = (uint16_t*)buffer->data;
        *write++ = COMPOSABLE_RAW_RUN;
        write++;
        const text_mode_font* font = current_font;
        text_buffer* screen = current_buffer;
        if (font->bytes_per_scan == 1)
            write = generate_line_byte_font(write, scanvideo_scanline_number(buffer->scanline_id), screen, font);
        else if (font->bytes_per_scan == 2)
            write = generate_line_short_font(write, scanvideo_scanline_number(buffer->scanline_id), screen, font);
        else
            assert(0);
        uint16_t* length = (uint16_t*)buffer->data + 1;
        length[0] = length[1];
        size_t count = write - length - 1;
        length[1] = count;
        if (count & 1) {
            *write++ = COMPOSABLE_EOL_ALIGN;
        } else {
            *write++ = COMPOSABLE_EOL_SKIP_ALIGN;
            *write++ = 0;
        }
        buffer->data_used = (uint32_t*)write - buffer->data;
        buffer->status = SCANLINE_OK;
        gpio_put(TIMING_MEASURE_PIN, 0);
        scanvideo_end_scanline_generation(buffer);
    }
}


////////////////////////////////////////////////////////////////////////////////
/////// MAIN ///////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/** Initialize a GPIO pin for output and set it to a default value. */
#define gpio_init_out(PIN, DEFAULT) gpio_init(PIN); gpio_set_dir(PIN, 1); gpio_put(PIN, DEFAULT)

int main()
{
    // Adjust CPU speed to be as fast as needed for selected options
#ifdef FULL_RES
    #if !TEXT_MODE_PALETTIZED_COLOR
        set_sys_clock_khz(180000, true);
    #else
        #ifndef CORE_1_IRQs
            set_sys_clock_khz(180000, true);
        #else
            set_sys_clock_khz(210000, true);
        #endif
    #endif
#else
    #if !TEXT_MODE_PALETTIZED_COLOR
        #ifndef CORE_1_IRQs
            set_sys_clock_khz(150000, true);
        #else
            set_sys_clock_khz(180000, true);
        #endif
    #else
        set_sys_clock_khz(180000, true);
    #endif
#endif
    stdio_init_all(); // Enable UART
    // Init GPIOs
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, 1);
    /// 800x480 TFT: Initialize control signals ////////////////////////////////
    gpio_init_out(MIRROR_PIN, 1);
    gpio_init_out(UPSIDE_DOWN_PIN, 0);
    gpio_init_out(RESET_PIN, 0);
    gpio_init_out(VGL_PIN, 0);
    gpio_init_out(AVDD_PIN, 0);
    gpio_init_out(VGH_PIN, 0);
    gpio_init_out(DITHERING_PIN, 1);
    gpio_init_out(TIMING_MEASURE_PIN, 0);
    gpio_set_function(PWM_PIN, GPIO_FUNC_PWM);
    unsigned slice = pwm_gpio_to_slice_num(PWM_PIN);
    pwm_set_wrap(slice, 16384);
    pwm_set_clkdiv_int_frac(slice, 2, 1);
    pwm_set_enabled(slice, true);
    pwm_set_gpio_level(PWM_PIN, 0);
    /// end 800x480 TFT ////////////////////////////////////////////////////////
    
    printf("\nHW init complete. ");

    // Display test text
    current_buffer->colors.foreground = BLACK;
    current_buffer->colors.background = BRIGHT_WHITE;
    const char* title = "The Picture of Dorian Gray";
    text_window title_window;
    text_window_ctor_in_place(&title_window, current_buffer, (coord){ 0, 0 },
        (coord){ current_buffer->size.x, 2 }
    );
#ifndef USE_CP437
    title_window.font = MONO_FONT_BOLD;
#endif
    text_window_put_string_centered_line(&title_window, "The Picture of Dorian Gray");
    text_window_newline_no_scroll(&title_window);
    text_window_put_string_centered_line(&title_window, "Oscar Wilde");
    text_window main_window;
    text_window_ctor_in_place(&main_window, current_buffer, (coord){ 0, title_window.size.y },
        (coord){ current_buffer->size.x, current_buffer->size.y - title_window.size.y }
    );
    text_window_put_string_word_wrap_partial(&main_window, 
        "Preface\n"
        "The artist is the creator of beautiful things.  To reveal art and conceal the artist is art"APS"s "
        "aim.  The critic is he who can translate into another manner or a new material his impression of "
        "beautiful things.\n"
        "The highest as the lowest form of criticism is a mode of autobiography.  Those who find ugly "
        "meanings in beautiful things are corrupt without being charming.  This is a fault.\n"
        "Those who find beautiful meanings in beautiful things are the cultivated.  For these there is hope.  "
        "They are the elect to whom beautiful things mean only beauty.\n"
        "There is no such thing as a moral or an immoral book.  Books are well written, or badly written.  "
        "That is all.\n"
        "The nineteenth century dislike of realism is the rage of Caliban seeing his own face in a glass.\n"
        "The nineteenth century dislike of romanticism is the rage of Caliban not seeing his own face in a "
        "glass.  The moral life of man forms part of the subject-matter of the artist, but the morality of "
        "art consists in the perfect use of an imperfect medium.  No artist desires to prove anything.  Even "
        "things that are true can be proved.  No artist has ethical sympathies.  An ethical sympathy in an "
        "artist is an unpardonable mannerism of style.  No artist is ever morbid.  The artist can express "
        "everything.  Thought and language are to the artist instruments of an art.  Vice and virtue are to "
        "the artist materials for an art.  From the point of view of form, the type of all the arts is the "
        "art of the musician.  From the point of view of feeling, the actor"APS"s craft is the type.  All art is "
        "at once surface and symbol.  Those who go beneath the surface do so at their peril.  Those who read "
        "the symbol do so at their peril.  It is the spectator, and not life, that art really mirrors.  "
        "Diversity of opinion about a work of art shows that the work is new, complex, and vital.  When "
        "critics disagree, the artist is in accord with himself.  We can forgive a man for making a useful "
        "thing as long as he does not admire it.  The only excuse for making a useless thing is that one "
        "admires it intensely.\n"
        "All art is quite useless.\n"
        "Chapter 1\n"
        "The studio was filled with the rich odour of roses, and when the light summer wind stirred amidst "
        "the trees of the garden, there came through the open door the heavy scent of the lilac, or the more "
        "delicate perfume of the pink-flowering thorn.\n"
        "From the corner of the divan of Persian saddle-bags on which he was lying, smoking, as was his "
        "custom, innumerable cigarettes, Lord Henry Wotton could just catch the gleam of the honey-sweet and "
        "honey-coloured blossoms of a laburnum, whose tremulous branches seemed hardly able to bear the "
        "burden of a beauty so flamelike as theirs; and now and then the fantastic shadows of birds in flight "
        "flitted across the long tussore-silk curtains that were stretched in front of the huge window,"
        "producing a kind of momentary Japanese effect, and making him think of those pallid, jade-faced "
        "painters of Tokyo who, through the medium of an art that is necessarily immobile, seek to convey the "
        "sense of swiftness and motion.  The sullen murmur of the bees shouldering their way through the long "
        "unmown grass, or circling with monotonous insistence round the dusty gilt horns of the straggling "
        "woodbine, seemed to make the stillness more oppressive.  The dim roar of London was like the bourdon "
        "note of a distant organ.\n"
        "In the centre of the room, clamped to an upright easel, stood the full-length portrait of a young "
        "man of extraordinary personal beauty, and in front of it, some little distance away, was sitting the "
        "artist himself, Basil Hallward, whose sudden disappearance some years ago caused, at the time, such "
        "public excitement and gave rise to so many strange conjectures.\n"
    );

    // Rainbow effect to prove color works.
    //text_cell* cell = text_buffer_cell(current_buffer, 0, 0);
    //for (int i = 0; i < current_buffer->size.x * current_buffer->size.y; i++, cell++) {
    //    //if ((i & 63) != BRIGHT_WHITE)
    //        cell->foreground = i;
    //        cell->background = ~i;
    //}

    /// 800x480 TFT: Wait for high voltages to come up and then sequence gating of analog power rails
    sleep_ms(10);
    gpio_put(RESET_PIN, 1);
    sleep_ms(20);
    gpio_put(VGL_PIN, 1);
    sleep_ms( 5 + 20); // I'm using relays for high-side switching, hence extra delays
    gpio_put(AVDD_PIN, 1);
    sleep_ms( 5 + 10);
    gpio_put(VGH_PIN, 1);
    sleep_ms(10 + 20);
    /// end 800x480 TFT ////////////////////////////////////////////////////////
#ifndef CORE_1_IRQs
    scanvideo_setup(&VIDEO_MODE);
    scanvideo_timing_enable(true);
#endif
    multicore_launch_core1(&run_video);
    /// 800x480 TFT: Wait for LCD to sync with data stream before turning the backlight on
    sleep_ms(200);
    pwm_set_gpio_level(PWM_PIN, 8192);
    /// end 800x480 TFT ////////////////////////////////////////////////////////

    const int loop_period = 50*1000; // 20 Hz
    absolute_time_t next_loop = make_timeout_time_us(loop_period);
    unsigned blinker = 0;
    while (1) {
        if (blinker++ % (1000*1000 / loop_period) >= 1000*1000 / loop_period / 2)
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
        else
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
        next_loop = delayed_by_us(next_loop, loop_period);
        // Put something interesting here.
        sleep_until(next_loop);
    }
}
