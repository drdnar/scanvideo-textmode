#include "text_mode.h"
#include "pico/scanvideo/scanvideo_base.h"

text_buffer* volatile text_mode_current_buffer;
const text_mode_font* volatile text_mode_current_font;
#if TEXT_MODE_PALETTIZED_COLOR
uint16_t* volatile text_mode_current_palette;
#endif

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
    register uint16_t* rpalette asm("r4") = text_mode_current_palette;
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


void CORE_1_FUNC(text_mode_render_loop)()
{
#if TEXT_MODE_CORE_1_IRQs
    scanvideo_setup(&TEXT_VIDEO_MODE);
    scanvideo_timing_enable(true);
#endif
    while (true) {
        struct scanvideo_scanline_buffer* buffer = scanvideo_begin_scanline_generation(true);
        gpio_put(TIMING_MEASURE_PIN, 1);
        uint16_t* write = (uint16_t*)buffer->data;
        *write++ = COMPOSABLE_RAW_RUN;
        write++;
        const text_mode_font* font = text_mode_current_font;
        text_buffer* screen = text_mode_current_buffer;
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
