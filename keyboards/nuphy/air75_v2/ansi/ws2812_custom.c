// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Key LED driver: the WS2812 data line (A7) is driven by SPI1 MOSI (AF0) with
 * DMA, so sending a frame neither blocks the main loop nor disables
 * interrupts (the bit-banged version did both for ~2.3ms per frame).
 *
 * Each WS2812 bit is encoded as 3 SPI bits at 3 MHz (333ns per SPI bit):
 * 0 -> 100 (333ns high), 1 -> 110 (667ns high), 1us per WS2812 bit.
 * Only MOSI is routed to a pin. SCK (A5), MISO (A6) and NSS (A4) of SPI1 are
 * matrix columns and stay GPIO inputs; the SPI master does not need them.
 */

#include "ws2812_driver.h"
#include "timer.h"

#define WS2812_SPI_DRIVER        SPID1
#define WS2812_SPI_MOSI_PAL_MODE 0

#if defined(WS2812_RGBW)
#    error "WS2812_RGBW is not supported by this driver"
#endif

#define WS2812_SPI_BYTES_PER_LED 9 // 24 WS2812 bits * 3 SPI bits / 8
#define WS2812_SPI_PREAMBLE      4
#define WS2812_SPI_RESET         (WS2812_TRST_US * 3 / 8 + 4) // zero bytes, 2.67us each
#define WS2812_SPI_BUFFER_SIZE   (WS2812_SPI_PREAMBLE + WS2812_LED_COUNT * WS2812_SPI_BYTES_PER_LED + WS2812_SPI_RESET)
// a frame takes ~2.3ms; anything far beyond that means the transfer is stuck
#define WS2812_SPI_TIMEOUT_MS    10

ws2812_led_t ws2812_leds[WS2812_LED_COUNT];
bool         flush_rgb_leds = 0;

static uint8_t       ws2812_spi_buffer[WS2812_SPI_BUFFER_SIZE];
static volatile bool ws2812_spi_busy = false;

static void ws2812_spi_done(SPIDriver *spip) {
    (void)spip;
    ws2812_spi_busy = false;
}

static const SPIConfig ws2812_spi_config = {
    .circular = false,
    .slave    = false,
    .data_cb  = ws2812_spi_done,
    .error_cb = NULL,
    .ssport   = PAL_PORT(WS2812_DI_PIN), // unused, no chip select
    .sspad    = PAL_PAD(WS2812_DI_PIN),
    .cr1      = SPI_CR1_BR_1 | SPI_CR1_BR_0, // fPCLK / 16 = 3 MHz
    .cr2      = 0,                           // 8 bit frames
};

// one color byte b7..b0 (MSB first) -> 3 SPI bytes: 1 b7 0, 1 b6 0, ... 1 b0 0
static inline void ws2812_encode_byte(uint8_t *dst, uint8_t b) {
    dst[0] = 0x92 | ((b & 0x80) >> 1) | ((b & 0x40) >> 3) | ((b & 0x20) >> 5);
    dst[1] = 0x49 | ((b & 0x10) << 1) | ((b & 0x08) >> 1);
    dst[2] = 0x24 | ((b & 0x04) << 5) | ((b & 0x02) << 3) | ((b & 0x01) << 1);
}

/**
 * @brief Route A7 to SPI1 MOSI. Also needed after deep sleep, which switches
 *        the pin to a GPIO output.
 */
void ws2812_pin_init(void) {
    palSetLineMode(WS2812_DI_PIN, PAL_MODE_ALTERNATE(WS2812_SPI_MOSI_PAL_MODE) | PAL_OUTPUT_TYPE_PUSHPULL | PAL_OUTPUT_SPEED_HIGHEST);
}

/**
 * @brief Wait until the previous frame has been sent (at most one frame, ~2.3ms).
 */
void ws2812_wait_idle(void) {
    uint16_t start = timer_read();
    while (ws2812_spi_busy) {
        if (timer_elapsed(start) > WS2812_SPI_TIMEOUT_MS) {
            spiStopTransfer(&WS2812_SPI_DRIVER, NULL);
            ws2812_spi_busy = false;
        }
    }
}

void ws2812_init(void) {
    ws2812_pin_init();
    spiStart(&WS2812_SPI_DRIVER, &ws2812_spi_config);
}

void ws2812_set_color(int index, uint8_t red, uint8_t green, uint8_t blue) {
    if (ws2812_leds[index].r != red || ws2812_leds[index].g != green || ws2812_leds[index].b != blue) {
        flush_rgb_leds = true;
    }
    ws2812_leds[index].r = red;
    ws2812_leds[index].g = green;
    ws2812_leds[index].b = blue;
}

void ws2812_set_color_all(uint8_t red, uint8_t green, uint8_t blue) {
    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        ws2812_set_color(i, red, green, blue);
    }
}

void ws2812_flush(void) {
    if (!flush_rgb_leds) { return; }

    // never rewrite the buffer while DMA is still reading it. With the RGB
    // matrix flush interval this only waits for back-to-back manual flushes.
    ws2812_wait_idle();

    uint8_t *dst = &ws2812_spi_buffer[WS2812_SPI_PREAMBLE];
    for (int i = 0; i < WS2812_LED_COUNT; i++, dst += WS2812_SPI_BYTES_PER_LED) {
#if (WS2812_BYTE_ORDER == WS2812_BYTE_ORDER_GRB)
        ws2812_encode_byte(dst, ws2812_leds[i].g);
        ws2812_encode_byte(dst + 3, ws2812_leds[i].r);
        ws2812_encode_byte(dst + 6, ws2812_leds[i].b);
#elif (WS2812_BYTE_ORDER == WS2812_BYTE_ORDER_RGB)
        ws2812_encode_byte(dst, ws2812_leds[i].r);
        ws2812_encode_byte(dst + 3, ws2812_leds[i].g);
        ws2812_encode_byte(dst + 6, ws2812_leds[i].b);
#elif (WS2812_BYTE_ORDER == WS2812_BYTE_ORDER_BGR)
        ws2812_encode_byte(dst, ws2812_leds[i].b);
        ws2812_encode_byte(dst + 3, ws2812_leds[i].g);
        ws2812_encode_byte(dst + 6, ws2812_leds[i].r);
#endif
    }

    ws2812_spi_busy = true;
    spiStartSend(&WS2812_SPI_DRIVER, sizeof(ws2812_spi_buffer), ws2812_spi_buffer);

    flush_rgb_leds = false;
}
