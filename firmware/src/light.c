/*
 * WS2812B Lights Control (Base + Left and Right Gimbals)
 * WHowe <github.com/whowechina>
 * 
 */

#include "light.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "bsp/board.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

#include "ws2812.pio.h"

#include "board_defs.h"
#include "config.h"

#define HID_TIMEOUT 10*1000*1000

static uint32_t buf_key[28*2];
static uint32_t buf_logo[6];

static inline uint32_t _rgb32(uint32_t c1, uint32_t c2, uint32_t c3, bool gamma_fix)
{
    if (gamma_fix) {
        c1 = ((c1 + 1) * (c1 + 1) - 1) >> 8;
        c2 = ((c2 + 1) * (c2 + 1) - 1) >> 8;
        c3 = ((c3 + 1) * (c3 + 1) - 1) >> 8;
    }

    return (c1 << 16) | (c2 << 8) | (c3 << 0);
}

uint32_t rgb32(uint32_t r, uint32_t g, uint32_t b, bool gamma_fix)
{
#if RGB_ORDER == GRB
    return _rgb32(g, r, b, gamma_fix);
#else
    return _rgb32(r, g, b, gamma_fix);
#endif
}

uint32_t rgb32_from_hsv(uint8_t h, uint8_t s, uint8_t v)
{
    uint32_t region, remainder, p, q, t;

    if (s == 0) {
        return v << 16 | v << 8 | v;
    }

    region = h / 43;
    remainder = (h % 43) * 6;

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            return v << 16 | t << 8 | p;
        case 1:
            return q << 16 | v << 8 | p;
        case 2:
            return p << 16 | v << 8 | t;
        case 3:
            return p << 16 | q << 8 | v;
        case 4:
            return t << 16 | p << 8 | v;
        default:
            return v << 16 | p << 8 | q;
    }
}

static inline uint32_t apply_level(uint32_t color, uint8_t level)
{
    unsigned r = (color >> 16) & 0xff;
    unsigned g = (color >> 8) & 0xff;
    unsigned b = color & 0xff;

    r = r * level / 255;
    g = g * level / 255;
    b = b * level / 255;

    return r << 16 | g << 8 | b;
}

void light_init()
{
    uint offset = pio_add_program(pio0, &ws2812_program);
    ws2812_program_init(pio0, 0, offset, RGB_PIN_KEY, 800000, false);
    ws2812_program_init(pio0, 1, offset, RGB_PIN_LOGO, 800000, false);
}

static void drive_led()
{
    for (int i = 0; i < count_of(buf_key); i++) {
        pio_sm_put_blocking(pio0, 0, buf_key[i] << 8u);
    }

    for (int i = 0; i < count_of(buf_logo); i++) {
        pio_sm_put_blocking(pio0, 1, buf_logo[i] << 8u);
    }
}

void light_update()
{
    static uint64_t last = 0;
    uint64_t now = time_us_64();
    if (now - last < 5000) { // 200Hz
        return;
    }

    last = now;

    drive_led();
}

static bool bypass_check(bool hid)
{
    static uint64_t hid_timeout = 0;
    uint64_t now = time_us_64();

    if (hid) {
        hid_timeout = now + HID_TIMEOUT;
        return false;
    }

    return now < hid_timeout;
}

void light_set_key(uint8_t index, uint32_t color, bool hid)
{
    if (bypass_check(hid)) {
        return;
    }

    if (index >= 28) {
        return;
    }

    buf_key[index * 2] = apply_level(color, nos_cfg->light.level_key);
    buf_key[index * 2 + 1] = apply_level(color, nos_cfg->light.level_key);
}

void light_set_logo(uint32_t color, bool hid)
{
    if (bypass_check(hid)) {
        return;
    }

    for (int i = 0; i < count_of(buf_logo); i++) {
        buf_logo[i] = apply_level(color, nos_cfg->light.level_logo);
    }
}