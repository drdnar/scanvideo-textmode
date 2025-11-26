#include "text_mode.h"
#include "pico/scanvideo/scanvideo_base.h"
#include "hardware/interp.h"

text_buffer* volatile text_mode_current_buffer;
const text_mode_font* volatile text_mode_current_font;
#if TEXT_MODE_PALETTIZED_COLOR
uint16_t* volatile text_mode_current_palette;
#endif


/**
 * Declares a function to be placed in the scratch Y RAM bank, which is usually dedicated to core 1's
 * stack.
 */
#define CORE_1_FUNC(FUNC_NAME) __scratch_y(#FUNC_NAME) FUNC_NAME


#define SHIFT_AMOUNT ((TEXT_MODE_MAX_FONT_WIDTH) <= 15 ? 17 : 17 - ((TEXT_MODE_MAX_FONT_WIDTH) - 15))


#if TEXT_MODE_MAX_FONT_WIDTH > 30
#error "TEXT_MODE_MAX_FONT_WIDTH must be less than 31."
#endif


void text_mode_setup_interp(void)
{
    interp_claim_lane_mask(interp1, 3);
    interp_config lane0 = interp_default_config();
    interp_config_set_shift(&lane0, 0);
    interp_config_set_mask(&lane0, SHIFT_AMOUNT, SHIFT_AMOUNT);
    interp_config_set_clamp(&lane0, true);
    interp_config_set_cross_result(&lane0, true);
    interp_set_config(interp1, 0, &lane0);
    interp_config lane1 = interp_default_config();
    interp_config_set_shift(&lane1, 1);
    interp_config_set_mask(&lane1, SHIFT_AMOUNT, 31);
    interp_set_config(interp1, 1, &lane1);
}


void text_mode_release_interp(void)
{
        interp_unclaim_lane_mask(interp1, 3);
}


void CORE_1_FUNC(text_mode_render_loop)()
{
#if TEXT_MODE_CORE_1_IRQs
    scanvideo_setup(&TEXT_VIDEO_MODE);
    scanvideo_timing_enable(true);
#endif
    text_mode_setup_interp();
    while (true) {
        struct scanvideo_scanline_buffer* buffer = scanvideo_begin_scanline_generation(true);
#ifdef TIMING_MEASURE_PIN
        gpio_put(TIMING_MEASURE_PIN, 1);
#endif
        uint16_t* write = (uint16_t*)buffer->data;
        *write++ = COMPOSABLE_RAW_RUN;
        write++;
        const text_mode_font* font = text_mode_current_font;
        text_buffer* screen = text_mode_current_buffer;
        write = text_mode_generate_line(write, scanvideo_scanline_number(buffer->scanline_id), screen, font);
        uint16_t* length = (uint16_t*)buffer->data + 1;
        length[0] = length[1];
        size_t count = write - length - 1 - 3;
        length[1] = count;
        if (count & 1) {
            *write++ = COMPOSABLE_EOL_SKIP_ALIGN;
            *write++ = 0;
        } else {
            *write++ = COMPOSABLE_EOL_ALIGN;
        }
        buffer->data_used = (uint32_t*)write - buffer->data;
        buffer->status = SCANLINE_OK;
        scanvideo_end_scanline_generation(buffer);
#ifdef TIMING_MEASURE_PIN
        gpio_put(TIMING_MEASURE_PIN, 0);
#endif
    }
}


uint16_t* CORE_1_FUNC(text_mode_generate_line)(uint16_t* write, unsigned scanline, text_buffer* screen, const text_mode_font* font)
{
    divmod_result_t r = hw_divider_divmod_u32(scanline, font->scan_lines);
    uint32_t char_row = to_remainder_u32(r);
    register int rjump_delta asm("r8") = 4 * (TEXT_MODE_MAX_FONT_WIDTH - font->scan_pixels) + 1; // +1 for Thumb mode
    register int rwrite_inc asm("r9") = font->scan_pixels * 2;
    register unsigned int embiggenationator asm("r10") = 0x1 << (SHIFT_AMOUNT - 1);
    register uint32_t rbytes asm("r1") = font->bytes_per_glyph;
    register text_cell* rread asm("r2") = text_buffer_cell(screen, 0, to_quotient_u32(r));
    register TEXT_MODE_FONT_DATA_TYPE* rfont asm("r3") = (TEXT_MODE_FONT_DATA_TYPE*)font->data + char_row;
    register uint32_t rcols asm("r4") = screen->size.x;
#if !TEXT_MODE_PALETTIZED_COLOR
    assert(sizeof(text_cell) == 6);
#else
    register uint16_t* rpalette asm("r6") = text_mode_current_palette;
    assert(sizeof(text_cell) == 4);
#endif
    asm volatile(
        // This uses the RP2040's interpolator's CLAMP mode to produce one of two possible values
        // for each input bit of the glyph's bitmap.
        // The basic idea is that BASE0 contains the background color,
        // while BASE1 is the foreground color with bit 16 set to make it very positive.
        // At each cycle in LANE1:
        //  - ACCUM1 is shifted one bit right, and the low 17 bits are masked away.
        //  - Then, BASE1 is added to form RESULT1.
        //     - This never overflows because of the previous masking.
        //  - RESULT1 is written back to ACCUM1.
        // Meanwhile in LANE0:
        //  - RESULT1 is fetched and every bit from RESULT1 is masked away except bit 17.
        //  - If bit 17 was clear, this value is zero, and is clamped to BASE0.
        //  - If bit 17 was set, this value is greater than BASE1 (whose highest set bit is bit 16),
        //    and is clamped to BASE1.
        // One input bit is unusable because it's the I'm-very-big bit of BASE1.
        // Register allocations:
        // r0: write pointer
        // r1: bytes per glyph
        // r2: read pointer
        // r3: font pointer
        // r4: column counter
        // r5: interpolator pointer
        // r6: palette pointer when applicable
        // r7: temp
        // r8: loop entry address
        // r9: write increment
        // r10: 0x00010000 (makes forground color bigger for interpolator clamp mode)
        "cellchar = 0\n"
        "cellfg = 2\n"
#if !TEXT_MODE_PALETTIZED_COLOR
        "cellbg = 4\n"
        "cellsize = 6\n"
#else
        "cellbg = 3\n"
        "cellsize = 4\n"
#endif
#if TEXT_MODE_MAX_FONT_WIDTH <= 15
        "shiftamount = 17\n"
#else
        "shiftamount = 17 - (TEXT_MODE_MAX_FONT_WIDTH - 15)"
#endif
        "accum0 = 0\n"
        "accum1 = 4\n"
        "base0 = 8\n"
        "base1 = 12\n"
        "pop0 = 20\n"
        "// Cache loop start address\n"
        "    adr     r7, loop_entry%=\n"
        "    add     %[loopstart], r7\n"
        "// Fetch colors\n"
#if !TEXT_MODE_PALETTIZED_COLOR
        "    ldrh    r7, [%[read], #cellfg]\n"
#else
        "    ldrb    r7, [%[read], #cellfg]\n"
        "    lsl     r7, r7, #1\n"
        "    ldrh    r7, [%[palette], r7]\n"
#endif
        "    add     r7, %[embiggener]\n"
        "    str     r7, [%[interp], #base1]\n"
#if !TEXT_MODE_PALETTIZED_COLOR
        "    ldrh    r7, [%[read], #cellbg]\n"
#else
        "    ldrb    r7, [%[read], #cellbg]\n"
        "    lsl     r7, r7, #1\n"
        "    ldrh    r7, [%[palette], r7]\n"
#endif
        "    str     r7, [%[interp], #base0]\n"
        "// Fetch character\n"
        "    ldrh    r7, [%[read], #cellchar]\n"
        "    add     %[read], %[read], #cellsize\n"
        "    // RP2040's CPU cores have the single-cycle multiplier option so shifting isn't any faster\n"
        "    // and is really only useful if you need to save a register.\n"
        "    mul     r7, %[glyphsize], r7\n"
#if TEXT_MODE_MAX_FONT_WIDTH <= 8
        "    ldrb    r7, [%[font], r7]\n"
#elif TEXT_MODE_MAX_FONT_WIDTH <= 16
        "    ldrh    r7, [%[font], r7]\n"
#else
        "    ldr     r7, [%[font], r7]\n"
#endif
        "    lsl     r7, r7, #shiftamount\n"
        "    str     r7, [%[interp], #accum0]\n"
        "    str     r7, [%[interp], #accum1]\n"
        "// Unrolled loop\n"
        "    bx      %[loopstart]\n"
        ".balign 4 // ADR requires 32-bit alignment\n"
        "loop_entry%=:"
#define handlebit(bit) "    ldr     r7, [%[interp], #pop0]\n" \
"    strh    r7, [%[write], #(" #bit " * 2)]\n"
#if TEXT_MODE_MAX_FONT_WIDTH > 29
        handlebit(29)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 28
        handlebit(28)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 27
        handlebit(27)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 26
        handlebit(26)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 25
        handlebit(25)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 24
        handlebit(24)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 23
        handlebit(23)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 22
        handlebit(22)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 21
        handlebit(21)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 20
        handlebit(20)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 19
        handlebit(19)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 18
        handlebit(18)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 17
        handlebit(17)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 16
        handlebit(16)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 15
        handlebit(15)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 14
        handlebit(14)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 13
        handlebit(13)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 12
        handlebit(12)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 11
        handlebit(11)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 10
        handlebit(10)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 9
        handlebit(9)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 8
        handlebit(8)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 7
        handlebit(7)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 6
        handlebit(6)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 5
        handlebit(5)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 4
        handlebit(4)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 3
        handlebit(3)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 2
        handlebit(2)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH > 1
        handlebit(1)
#endif
#if TEXT_MODE_MAX_FONT_WIDTH < 1
#error "TEXT_MODE_MAX_FONT_WIDTH is less than one???"
#endif
        handlebit(0)
        "    add     %[write], %[writeinc]\n"
        "    sub     %[cols], #1\n"
        "    beq     done%=\n"
        "// Fetch colors\n"
#if !TEXT_MODE_PALETTIZED_COLOR
        "    ldrh    r7, [%[read], #cellfg]\n"
#else
        "    ldrb    r7, [%[read], #cellfg]\n"
        "    lsl     r7, r7, #1\n"
        "    ldrh    r7, [%[palette], r7]\n"
#endif
        "    add     r7, %[embiggener]\n"
        "    str     r7, [%[interp], #base1]\n"
#if !TEXT_MODE_PALETTIZED_COLOR
        "    ldrh    r7, [%[read], #cellbg]\n"
#else
        "    ldrb    r7, [%[read], #cellbg]\n"
        "    lsl     r7, r7, #1\n"
        "    ldrh    r7, [%[palette], r7]\n"
#endif
        "    str     r7, [%[interp], #base0]\n"
        "// Fetch character\n"
        "    ldrh    r7, [%[read], #cellchar]\n"
        "    add     %[read], %[read], #cellsize\n"
        "    mul     r7, %[glyphsize], r7\n"
#if TEXT_MODE_MAX_FONT_WIDTH <= 8
        "    ldrb    r7, [%[font], r7]\n"
#elif TEXT_MODE_MAX_FONT_WIDTH <= 16
        "    ldrh    r7, [%[font], r7]\n"
#else
        "    ldr     r7, [%[font], r7]\n"
#endif
        "    lsl     r7, r7, #shiftamount\n"
        "    str     r7, [%[interp], #accum0]\n"
        "    str     r7, [%[interp], #accum1]\n"
        "    bx      %[loopstart]\n"
        "done%=:"
     :  [read]     "=r" (rread),
        [write]    "=r" (write),
        [cols]     "=r" (rcols),
        [loopstart]"=r" (rjump_delta)
     : "[write]"        (write),
        [glyphsize]"r"  (rbytes),
       "[read]"         (rread),
        [font]     "r"  (rfont),
       "[cols]"    "r"  (rcols),
        [interp]   "r"  (interp1_hw),
#if TEXT_MODE_PALETTIZED_COLOR
        [palette]  "r"  (rpalette),
#endif
       "[loopstart]""r" (rjump_delta),
        [writeinc] "r"  (rwrite_inc),
        [embiggener]"r" (embiggenationator)
     : "cc", "memory", "r7"
    );
#undef resetbit
#undef sethighbit
#undef setbit
    return write;
}
