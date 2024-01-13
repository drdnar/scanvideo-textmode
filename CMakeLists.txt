cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)
include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

project(scanvideotest C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
pico_sdk_init()

add_executable(scanvideotest
    main.c
)

set(PICO_NO_FLASH 1)

target_sources(scanvideotest PUBLIC
    main.c
    text_buffer.c
    text_window.c
    text_mode.c
    monofonts12_normal.c
    cp437.c
)

target_compile_definitions(scanvideotest PRIVATE
    PICO_DEFAULT_UART=0
    PICO_DEFAULT_UART_TX_PIN=16
    PICO_DEFAULT_UART_RX_PIN=17
)

target_compile_definitions(scanvideotest PUBLIC
    # Standard values are 8, 15, 16, and 30.
    # If <= 8, each line of a font takes one byte; <= 16, two bytes; otherwise, four bytes.
    # Due to the way the interpolator decodes colors,
    # each pixel wider than 15 eats into the bits usable for color.
    # For example, with 19, colors are limited to 12 bits.
    # This applies even if you're using bit 15 as a transparency bit.
    TEXT_MODE_MAX_FONT_WIDTH=8
    # If set to 1, text cell colors are 8-bit indexes into an array of 16-bit color values.
    # This will consume about 10 % more CPU time.
    TEXT_MODE_PALETTIZED_COLOR=0
    # Set to run IRQs on core 1 along side to scan line generation code.
    TEXT_MODE_CORE_1_IRQs=0
    # Name of video mode to choose.
    TEXT_VIDEO_MODE=tft_mode_480x800_60
    # These values are used to compute the correct text buffer size.
    SCREEN_WIDTH=800
    SCREEN_HEIGHT=480
    # Used to measure CPU usage with an oscilloscope.
    TIMING_MEASURE_PIN=28
    PICO_SCANVIDEO_MAX_SCANLINE_BUFFER_WORDS=1024
    PICO_SCANVIDEO_SCANLINE_BUFFER_COUNT=4
    PICO_SCANVIDEO_COLOR_PIN_BASE=0
    PICO_SCANVIDEO_COLOR_PIN_COUNT=12
    PICO_SCANVIDEO_SYNC_PIN_BASE=12
    PICO_SCANVIDEO_ENABLE_CLOCK_PIN=1
    PICO_SCANVIDEO_ENABLE_DEN_PIN=1
    PICO_SCANVIDEO_PIXEL_RSHIFT=0
    PICO_SCANVIDEO_PIXEL_RCOUNT=2
    PICO_SCANVIDEO_PIXEL_GSHIFT=2
    PICO_SCANVIDEO_PIXEL_GCOUNT=2
    PICO_SCANVIDEO_PIXEL_BSHIFT=4
    PICO_SCANVIDEO_PIXEL_BCOUNT=2
)

pico_enable_stdio_uart(scanvideotest 1)

pico_add_extra_outputs(scanvideotest)

target_link_libraries(scanvideotest
    hardware_dma
    hardware_sync
    hardware_pio
    hardware_clocks
    hardware_gpio
    hardware_pwm
    hardware_divider
    hardware_adc
    hardware_vreg
    hardware_interp
    pico_stdlib
    pico_time
    pico_unique_id
    pico_divider
    pico_int64_ops
    pico_multicore
    pico_scanvideo
    pico_scanvideo_dpi
)
