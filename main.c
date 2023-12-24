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
#include "monofonts12.h"
#include "cp437.h"

// Implement full 800x480 at 180 MHz instead of 760x480 at 150 MHz.
//#define FULL_RES
// Use CP437 font instead
#define USE_CP437

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
#define LDQ ""LDQ""
/** Right double-quote */
#define RDQ ""RDQ""
/** Apostrophe */
#define APS ""APS""
/** Em dash */
#define EMD ""EMD""
#endif

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


#define HEIGHT 480
#define WIDTH 800


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

//#ifndef USE_CP437
#define TEXT_ROWS 40
#define TEXT_COLS 100
//#else
//#define TEXT_ROWS 35
//#define TEXT_COLS 88
//#endif

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
    .clock_polarity = 1,

    .enable_den = 0
};
/** Custom mode for my TFT LCD. */
const scanvideo_mode_t tft_mode_480x800_60 = {
    .default_timing = &tft_timing_480x800_60_default,
    .pio_program = &video_24mhz_composable, // ??
    .width = WIDTH,
    .height = HEIGHT,
    .xscale = 1,
    .yscale = 1,
    .yscale_denominator = 1
};


#ifdef FULL_RES
#define COLS_TO_RENDER (current_buffer->size.x)
#else
#ifndef USE_CP437
#define COLS_TO_RENDER 94
#else /* USE_CP437 is set */
#define COLS_TO_RENDER 88
#endif /* USE_CP437 */
#endif /* FULL_RES */

/** Rendering routine. */
__scratch_y("renderloop") void renderloop()
{
    while (true) {
        struct scanvideo_scanline_buffer* buffer = scanvideo_begin_scanline_generation(true);
        gpio_put(TIMING_MEASURE_PIN, 1);
        uint16_t* write = (uint16_t*)buffer->data;
        *write++ = COMPOSABLE_RAW_RUN;
        /*uint16_t* length = */write++;
        const text_mode_font* font = current_font;
        // Assembly code depends on layout of text_cell.
        assert(sizeof(text_cell) == 6);
        if (font->bytes_per_scan == 1) {
            // Render loop
            divmod_result_t r = hw_divider_divmod_u32(scanvideo_scanline_number(buffer->scanline_id), font->scan_lines);
            uint32_t char_row = to_remainder_u32(r);
            uint32_t ignored1, ignored2, ignored3;
            register uint32_t row asm("r5") = to_quotient_u32(r);
            register text_color* rwrite asm("r0") = write;
            register uint32_t rbytes asm("r1") = font->bytes_per_glyph;
            register text_cell* rread asm("r2") = text_buffer_cell(current_buffer, 0, row);
            register uint8_t* rfont asm("r3") = (uint8_t*)font->data + char_row;
            register uint32_t rcols asm("r4") = COLS_TO_RENDER;
            register int rjump_delta asm("r8") = 6 * (8 - font->scan_pixels) + 1; // +1 for Thumb mode
            register int rwrite_inc asm("r9") = font->scan_pixels * 2;
        asm volatile(
            // This ends up being a bit over 4-5 cycles per pixel depending on how often it changes
            // between foreground and background color.  Since Latin scripts are mostly white space,
            // most time will be spent in the reset bit path, where there are no branches to flush
            // the (short two-stage) pipeline, so it'll average closer to 4 cycles per pixel.
            // But some semigraphical glyphs can alternate every pixel, so you still should plan for
            // the worst-case 5 cycles.
            // TODO: Still possibly some cycles to be saved by copying the loop start stuff to end of
            // the loop so as to get rid of one jump per character.
            "// Cache loop start address\n"
            "    adr     r6, loop_entry%=\n"
            "    add     %[loopstart], r6\n"
//            "loop%=:\n"
            "// Fetch character and colors\n"
            "    ldrh    %[bitmap], [%[read], #0]\n"
            "    // RP2040's CPU cores have the single-cycle multiplier option so shifting isn't any faster\n"
            "    // and is really only useful if you need to save a register.\n"
            "    mul     %[bitmap], %[glyphsize], %[bitmap]\n"
            "    ldrb    %[bitmap], [%[bitmap], %[font]]\n"
            "    ldrh    r6, [%[read], #2]\n" // offset of text_cell.foreground
            "    ldrh    r7, [%[read], #4]\n" // offset of text_cell.background
            "    add     %[read], %[read], #6\n" // sizeof(text_cell)
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
            "    sub     %[cols], #1\n"
//            "    bne     loop%=\n"
//            "    b       done%=\n"
            "    beq     done%=\n"
            "    ldrh    %[bitmap], [%[read], #0]\n"
            "    mul     %[bitmap], %[glyphsize], %[bitmap]\n"
            "    ldrb    %[bitmap], [%[bitmap], %[font]]\n"
            "    ldrh    r6, [%[read], #2]\n" // offset of text_cell.foreground
            "    ldrh    r7, [%[read], #4]\n" // offset of text_cell.background
            "    add     %[read], %[read], #6\n" // sizeof(text_cell)
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
            "    sub     %[cols], #1\n"
//            "    bne     loop%=\n"
            "    beq     done%=\n"
            "    ldrh    %[bitmap], [%[read], #0]\n"
            "    mul     %[bitmap], %[glyphsize], %[bitmap]\n"
            "    ldrb    %[bitmap], [%[bitmap], %[font]]\n"
            "    ldrh    r6, [%[read], #2]\n" // offset of text_cell.foreground
            "    ldrh    r7, [%[read], #4]\n" // offset of text_cell.background
            "    add     %[read], %[read], #6\n" // sizeof(text_cell)
            "    bx      %[loopstart]\n"
            "done%=:"
        :  [read]     "=r" (ignored1),
           [write]    "=r" (write),
           [cols]     "=r" (ignored2),
           [loopstart]"=r" (rjump_delta),
           [bitmap]   "=r" (ignored3)
        : "[write]"        (rwrite),
           [glyphsize]"r"  (rbytes),
          "[read]"         (rread),
           [font]     "r"  (rfont),
          "[loopstart]""r" (rjump_delta),
           [writeinc] "r"  (rwrite_inc),
          "[bitmap]"  "r"  (row),
          "[cols]"    "r"  (rcols)
        : "cc", "memory",
           "r6", "r7"
        );
#undef resetbit
#undef sethighbit
#undef setbit
        } else if (font->bytes_per_scan == 2) {
            // Render loop
            divmod_result_t r = hw_divider_divmod_u32(scanvideo_scanline_number(buffer->scanline_id), font->scan_lines);
            uint32_t char_row = to_remainder_u32(r);
            uint32_t ignored1, ignored2, ignored3;
            register uint32_t row asm("r5") = to_quotient_u32(r);
            register text_color* rwrite asm("r0") = write;
            register uint32_t rbytes asm("r1") = font->bytes_per_glyph;
            register text_cell* rread asm("r2") = text_buffer_cell(current_buffer, 0, row);
            register uint16_t* rfont asm("r3") = (uint16_t*)font->data + char_row;
            register uint32_t rcols asm("r4") = COLS_TO_RENDER;
            register int rjump_delta asm("r8") = 6 * (16 - font->scan_pixels) + 1; // +1 for Thumb mode
            register int rwrite_inc asm("r9") = font->scan_pixels * 2;
        asm volatile(
            // This ends up being a bit over 4-5 cycles per pixel depending on how often it changes
            // between foreground and background color.  Since Latin scripts are mostly white space,
            // most time will be spent in the reset bit path, where there are no branches to flush
            // the (short two-stage) pipeline, so it'll average closer to 4 cycles per pixel.
            // But some semigraphical glyphs can alternate every pixel, so you still should plan for
            // the worst-case 5 cycles.
            "// Cache loop start address\n"
            "    adr     r6, loop_entry%=\n"
            "    add     %[loopstart], r6\n"
//            "loop%=:\n"
            "// Fetch character and colors\n"
            "    ldrh    %[bitmap], [%[read], #0]\n"
            "    // RP2040's CPU cores have the single-cycle multiplier option so shifting isn't any faster\n"
            "    // and is really only useful if you need to save a register.\n"
            "    mul     %[bitmap], %[glyphsize], %[bitmap]\n"
            "    ldrh    %[bitmap], [%[bitmap], %[font]]\n"
            "    ldrh    r6, [%[read], #2]\n" // offset of text_cell.foreground
            "    ldrh    r7, [%[read], #4]\n" // offset of text_cell.background
            "    add     %[read], %[read], #6\n" // sizeof(text_cell)
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
            "    sub     %[cols], #1\n"
//            "    bne     loop%=\n"
//            "    b       done%=\n"
            "    beq     done%=\n"
            "    ldrh    %[bitmap], [%[read], #0]\n"
            "    mul     %[bitmap], %[glyphsize], %[bitmap]\n"
            "    ldrh    %[bitmap], [%[bitmap], %[font]]\n"
            "    ldrh    r6, [%[read], #2]\n" // offset of text_cell.foreground
            "    ldrh    r7, [%[read], #4]\n" // offset of text_cell.background
            "    add     %[read], %[read], #6\n" // sizeof(text_cell)
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
            "    sub     %[cols], #1\n"
//            "    bne     loop%=\n"
            "    beq     done%=\n"
            "    ldrh    %[bitmap], [%[read], #0]\n"
            "    mul     %[bitmap], %[glyphsize], %[bitmap]\n"
            "    ldrh    %[bitmap], [%[bitmap], %[font]]\n"
            "    ldrh    r6, [%[read], #2]\n" // offset of text_cell.foreground
            "    ldrh    r7, [%[read], #4]\n" // offset of text_cell.background
            "    add     %[read], %[read], #6\n" // sizeof(text_cell)
            "    bx      %[loopstart]\n"
            "done%=:"
        :  [read]     "=r" (ignored1),
           [write]    "=r" (write),
           [cols]     "=r" (ignored2),
           [loopstart]"=r" (rjump_delta),
           [bitmap]   "=r" (ignored3)
        : "[write]"        (write),
           [glyphsize]"r"  (rbytes),
          "[read]"         (rread),
           [font]     "r"  (rfont),
          "[loopstart]""r" (rjump_delta),
           [writeinc] "r"  (rwrite_inc),
          "[bitmap]"  "r"  (row),
          "[cols]"    "r"  (rcols)
        : "cc", "memory",
          "r6", "r7"
        );
#undef resetbit
#undef sethighbit
#undef setbit
        } else {
            assert(0);
        }
        uint16_t* length = (uint16_t*)buffer->data + 1;
        length[0] = length[1];
        short count = font->scan_pixels * COLS_TO_RENDER;
        length[1] = count;
        // Buffer footer
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
    

void run_video()
{
    renderloop();
}


/** Initialize a GPIO pin for output and set it to a default value. */
#define gpio_init_out(PIN, DEFAULT) gpio_init(PIN); gpio_set_dir(PIN, 1); gpio_put(PIN, DEFAULT)

int main()
{
#ifdef FULL_RES
    set_sys_clock_khz(180000, true);
#else
    set_sys_clock_khz(150000, true);
#endif
    stdio_init_all(); // Enable UART
    // Init GPIOs
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, 1);
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
    
    printf("\nHW init complete. ");

    current_buffer->colors.foreground = BLACK;
    current_buffer->colors.background = BRIGHT_WHITE;
#ifndef USE_CP437
    current_buffer->font = MONO_FONT_BOLD;
#else
    main_buffer.font = 0;
#endif
    text_buffer_put_string(current_buffer,
    //   00        10        20        30        40        50        60        70        80        90
    //  <123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 >
        "                                            I Have a Dream                                          "
    );
#ifndef USE_CP437
    main_buffer.font = MONO_FONT_REGULAR;
#endif
    text_buffer_put_string(current_buffer,
        "I am happy to join with you today in what will go down in history as the greatest demonstration for " //  1
        "freedom in the history of our nation.                                                               " //  2
        "Five score years ago, a great American, in whose symbolic shadow we stand today, signed the         " //  3
        "Emancipation Proclamation.  This momentous decree came as a great beacon light of hope to millions  " //  4
        "of Negro slaves who had been seared in the flames of withering injustice.  It came as a joyous      " //  5
        "daybreak to end the long night of their captivity.                                                  " //  6
        "But 100 years later, the Negro still is not free.  One hundred years later, the life of the Negro is" //  7
        "still sadly crippled by the manacles of segregation and the chains of discrimination.  One hundred  " //  8
        "years later, the Negro lives on a lonely island of poverty in the midst of a vast ocean of material " //  9
        "prosperity.  One hundred years later, the Negro is still languished in the corners of American      " // 10
        "society and finds himself an exile in his own land.                                                 " // 11
        "And so we"APS"ve come here today to dramatize the same cruel condition.  In a sense we"APS"ve come to our   " // 12
        "nation"APS"s capital to cash a cheque.  When the architects of our republic wrote the magnificent words " // 13
        "of the Constitution and the Declaration of Independence, they were signing a promissory note to     " // 14
        "which every American was to fall heir.  This note was a promise that all men"EMD"yes, black men as well " // 15
        "as white men"EMD"would be guaranteed the inalienable rights of "LDQ"Life, Liberty, and the pursuit of       " // 16
        "Happiness."RDQ"                                                                                         " // 17
        "It is obvious today that America has defaulted on this promissory note insofar as her citizens of   " // 18
        "color are concerned.  Instead of honoring this sacred obligation, America has given the Negro people" // 19
        "a bad cheque, a cheque which has come back marked "LDQ"insufficient funds."RDQ"  But we refuse to believe   " // 20
        "that the bank of justice is bankrupt.  We refuse to believe that there are insufficient funds in the" // 21
        "great vaults of opportunity of this nation.  So we"APS"ve come to cash this cheque"EMD"a cheque that will   " // 22
        "give us upon demand the riches of freedom and the security of justice.                              " // 23
        "We have also come to this hallowed spot to remind America of the fierce urgency of now.  This is no " // 24
        "time to engage in the luxury of cooling off or to take the tranquilizing drug of gradualism.  Now is" // 25
        "the time to make real the promises of democracy.  Now is the time to rise from the dark and desolate" // 26
        "valley of segregation to the sunlit path of racial justice.  Now is the time to lift our nation from" // 27
        "the quicksands of racial injustice to the solid rock of brotherhood.  Now is the time to make       " // 28
        "justice a reality for all of God"APS"s children.                                                        " // 29
        "It would be fatal for the nation to overlook the urgency of the moment.  This sweltering summer of  " // 30
        "the Negro"APS"s legitimate discontent will not pass until there is an invigorating autumn of freedom and" // 31
        "equality.  1963 is not an end, but a beginning.  Those who hope that the Negro needed to blow off   " // 32
        "steam and will now be content will have a rude awakening if the nation returns to business as usual." // 33
        "There will be neither rest nor tranquillity in America until the Negro is granted his citizenship   " // 34
        "rights.  The whirlwinds of revolt will continue to shake the foundations of our nation until the    " // 35
        "bright day of justice emerges.                                                                      " // 36
        "But there is something that I must say to my people, who stand on the warm threshold which leads    " // 37
        "into the palace of justice: in the process of gaining our rightful place we must not be guilty of   " // 38
        "wrongful deeds.  Let us not seek to satisfy our thirst for freedom by drinking from the cup of      " // 39
        //"bitterness and hatred.  We must forever conduct our struggle on the high plane of dignity and       " // 40
        //"discipline.  We must not allow our creative protest to degenerate into physical violence.  Again and" // 41
        //"again we must rise to the majestic heights of meeting physical force with soul force.               " // 42
    );
    // Rainbow effect to prove color works.
    //text_cell* cell = text_buffer_cell(current_buffer, 0, 0);
    //for (int i = 0; i < current_buffer->size.x * current_buffer->size.y; i++, cell++) {
    //    //if ((i & 63) != BRIGHT_WHITE)
    //        cell->foreground = i;
    //        cell->background = ~i;
    //}
    // Wait for high voltages to come up and then sequence gating of analog power rails
    sleep_ms(10);
    gpio_put(RESET_PIN, 1);
    sleep_ms(20);
    gpio_put(VGL_PIN, 1);
    sleep_ms( 5 + 20); // I'm using relays for high-side switching, hence extra delays
    gpio_put(AVDD_PIN, 1);
    sleep_ms( 5 + 10);
    gpio_put(VGH_PIN, 1);
    sleep_ms(10 + 20);
    scanvideo_setup(&tft_mode_480x800_60);
    //scanvideo_setup(&vga_mode_640x480_60);
    scanvideo_timing_enable(true);
    multicore_launch_core1(&run_video);
    sleep_ms(200);
    pwm_set_gpio_level(PWM_PIN, 8192);

    const int loop_period = 4*1000;//1000000 / lcd_config.frame_rate;//4*1000;
    absolute_time_t next_loop = make_timeout_time_us(loop_period);
    unsigned blinker = 0;
    while (1) {
        if (blinker++ % (1000*1000 / loop_period) >= 1000*1000 / loop_period / 2)
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
        else
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
        next_loop = delayed_by_us(next_loop, loop_period);
        sleep_until(next_loop);
    }
}