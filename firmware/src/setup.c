/*
 * Live Setup
 * WHowe <github.com/whowechina>
 * 
 */

#include "setup.h"

#include <stdint.h>
#include <stdbool.h>

#include "hardware/timer.h"

#include "config.h"
#include "savedata.h"
#include "hammer.h"
#include "light.h"
#include "button.h"

enum {
    SETUP_NONE = 0,
    SETUP_LEVEL_KEY,
    SETUP_LEVEL_LOGO,
    SETUP_LIGHT_MODE,
    SETUP_BUTTON,
    SETUP_HID,
    SETUP_MAX,
};

#define MODE_COUNT (SETUP_MAX - 1)

static nos_cfg_t cfg_bak;
static uint8_t level_key;
static uint8_t level_logo;

static int setup_mode = SETUP_NONE;
static uint16_t last_pressed;
static uint16_t pressed;
static uint16_t just_pressed;
static bool temp_locked = false;

static uint8_t fast_breath;
static uint8_t slow_breath;

#define B1_PREV 1
#define B2_NEXT 2
#define B3_NO 4
#define B4_YES 8

static void read_button()
{
    last_pressed = pressed;
    pressed = button_read();
    just_pressed = pressed & ~last_pressed;
}

static void run_mode_none()
{
    if (pressed == (B1_PREV | B2_NEXT | B3_NO | B4_YES)) {
        cfg_bak.light = nos_cfg->light;
        cfg_bak.hid = nos_cfg->hid;
        level_key = nos_cfg->light.level_key;
        level_logo = nos_cfg->light.level_logo;
        nos_cfg->light.level_key = 240;
        nos_cfg->light.level_logo = 240;
        setup_mode = SETUP_LEVEL_KEY;
        temp_locked = true;
    }
}

static void run_nav_lights()
{
    for (int i = 0; i < MODE_COUNT; i++) {
        uint8_t h = (i * 255) / MODE_COUNT;
        uint8_t s = 30;
        uint8_t v = 16;

        if (setup_mode == i + 1) {
            v = slow_breath;
            s = 255;
        }

        uint32_t color = rgb32_from_hsv(h, s, v);
        light_set_sublogo(i, color);
    }

    for (int i = MODE_COUNT; i < 6; i++) {
        light_set_sublogo(i, 0);
    }
}

static void run_navigation()
{
    if (temp_locked && pressed) {
        return;
    }
    temp_locked = false;

    if (just_pressed & B1_PREV) {
        setup_mode = (setup_mode + MODE_COUNT - 2) % MODE_COUNT + 1;
    }
    if (just_pressed & B2_NEXT) {
        setup_mode = setup_mode % MODE_COUNT + 1;
    }
    if (just_pressed & B3_NO) {
        setup_mode = SETUP_NONE;
        nos_cfg->light = cfg_bak.light;
        nos_cfg->hid = cfg_bak.hid;
        return;
    }
    if (just_pressed & B4_YES) {
        setup_mode = SETUP_NONE;
        nos_cfg->light.level_key = level_key;
        nos_cfg->light.level_logo = level_logo;
        savedata_save(true);
        return;
    }

    run_nav_lights();
}


// Curve: y = k * x ^ 1.9
static const uint8_t level_map[28] = {
    0, 2, 4, 7, 10, 14, 18, 24, 30, 36, 43, 51, 59, 68, 78, 88,
    99, 110, 122, 135, 148, 161, 175, 190, 206, 222, 238, 255
};

static void run_level(bool key_or_logo)
{
    for (int i = 0; i < 28; i++) {
        uint8_t step = level_map[i];
        if (hammer_pressed(i)) {
            if (key_or_logo) {
                level_key = step;
            } else {
                level_logo = step;
            }
            break;
        }
    }

    uint8_t level = key_or_logo ? level_key : level_logo;

    for (int i = 0; i < 28; i++) {
        uint8_t step = level_map[i];
        uint32_t color = 0;
        if (level >= step) {
            color = rgb32_from_hsv(key_or_logo ? 0 : 51, 120, step);        
        }
        light_set_subkey(i, false, color);
        light_set_subkey(i, true, color);
    }
}

static void clear_key_lights()
{
    for (int i = 0; i < 28; i++) {
        light_set_subkey(i, false, 0);
        light_set_subkey(i, true, 0);
    }
}

static void run_light_mode()
{
    for (int i = 0; i < 4; i++) {
        if (hammer_pressed(i * 2) || hammer_pressed(i * 2 + 1)) {
            nos_cfg->light.type = i;
            break;
        }
    }

    for (int i = 0; i < 4; i++) {
        static const uint32_t colors[4] = {
            0x202020, 0x401010, 0x104010, 0x101040
        };
        light_set_subkey(i * 2, false, colors[i]);
        light_set_subkey(i * 2 + 1, false, colors[i]);

        uint32_t color = 0;
        if (i == nos_cfg->light.type) {
            color = rgb32_from_hsv(102, 255, slow_breath);
        }
        light_set_subkey(i * 2, true, color);
        light_set_subkey(i * 2 + 1, true, color);
    }

    for (int i = 8; i < 28; i++) {
        light_set_subkey(i, false, 0);
        light_set_subkey(i, true, 0);
    }
}

static void run_button()
{
    clear_key_lights();
}

static void run_hid()
{
    clear_key_lights();
}

static void run_mode()
{
    if (setup_mode == SETUP_NONE) {
        run_mode_none();
    } else if (setup_mode == SETUP_LEVEL_KEY) {
        run_level(true);
    } else if (setup_mode == SETUP_LEVEL_LOGO) {
        run_level(false);
    } else if (setup_mode == SETUP_LIGHT_MODE) {
        run_light_mode();
    } else if (setup_mode == SETUP_BUTTON) {
        run_button();
    } else if (setup_mode == SETUP_HID) {
        run_hid();
    }
}

static const uint8_t breath_level[] = {
    0, 0, 1, 2, 4, 6, 9, 12, 16, 21, 26, 33, 40, 47, 55,
    65, 74, 85, 96, 109, 122, 135, 150, 165, 182, 
    182, 165, 150, 135, 122, 109, 96, 85, 74, 65, 55, 47,
    40, 33, 26, 21, 16, 12, 9, 6, 4, 2, 1, 0, 0
};

static void update_masks()
{
    slow_breath = breath_level[(time_us_32() / 4777) % count_of(breath_level)];
    fast_breath = breath_level[(time_us_32() / 3777) % count_of(breath_level)];
}

void setup_update()
{
    read_button();
    run_mode();
    if (setup_mode != SETUP_NONE) {
        run_navigation();
    }
    update_masks();
}

bool setup_is_active()
{
    return setup_mode != SETUP_NONE;
}
