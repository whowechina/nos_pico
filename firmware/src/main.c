/*
 * Controller Main
 * WHowe <github.com/whowechina>
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

#include "tusb.h"
#include "usb_descriptors.h"

#include "board_defs.h"

#include "savedata.h"
#include "config.h"
#include "cli.h"
#include "commands.h"

#include "light.h"
#include "hammer.h"
#include "button.h"

#define WHITE 0xffffff

static void run_lights()
{
    uint32_t phase = time_us_32() / 50000;
    light_set_logo(rgb32_from_hsv(phase, 255, 255), false);

    for (int i = 0; i < hammer_keynum(); i++) {
        uint32_t color = 0;
        if (nos_cfg->light.type == 1) {
            color = hammer_pressed(i) ? 0x808080 : 0;
        } else if (nos_cfg->light.type == 2) {
            color = rgb32_from_hsv(0, 0, 255 - hammer_analog(i));
        } else if (nos_cfg->light.type == 3) {
            if (hammer_pressed(i)) {
                uint16_t vel = hammer_velocity(i);
                if (vel > 254) {
                    vel = 254;
                }
                color = rgb32_from_hsv(0, 0, vel + 1);
            }
        }
        light_set_key(i, color, false);
    }
}

static mutex_t core1_io_lock;
static void core1_loop()
{
    sleep_ms(500);
    while (1) {
        if (mutex_try_enter(&core1_io_lock, NULL)) {
            run_lights();
            light_update();
            mutex_exit(&core1_io_lock);
        }
        cli_fps_count(1);
        sleep_us(700);
    }
}

struct __attribute__((packed)) {
    uint32_t buttons;
    uint8_t analog[28];
} hid_report, old_hid_report;

static void hid_update()
{
    memset(&hid_report, 0, sizeof(hid_report));

    if (nos_cfg->hid.button) {
        for (int i = 0; i < hammer_keynum(); i++){
            hid_report.buttons |= hammer_pressed(i) ? (1 << i) : 0;
        }
    }

    hid_report.buttons |= button_read() << 28;

    for (int i = 0; i < hammer_keynum(); i++) {
        hid_report.analog[i] = nos_cfg->hid.analog ? hammer_analog(i) : 127;
    }

    if (!tud_hid_ready()) {
        return;
    }
    
    if (memcmp(&hid_report, &old_hid_report, sizeof(hid_report)) == 0) {
        return;
    }

    if (tud_hid_report(REPORT_ID_JOYSTICK, &hid_report, sizeof(hid_report))) {
        old_hid_report = hid_report;
    }
}

static void proc_midi()
{
    if (!nos_cfg->hid.midi) {
        return;
    }

    static uint8_t key_pitch[7] = { 0, 2, 4, 5, 7, 9, 11 };
    for (int i = 0; i < hammer_keynum(); i++) {
        if (hammer_updated(i)) {
            bool on = hammer_pressed(i);
            uint16_t vel = hammer_velocity(i);
            uint8_t note = 36 + (i / 7) * 12 + key_pitch[i % 7];

            if (vel <= 0) {
                vel = 1;
            }
            if (vel > 127) {
                vel = 127;
            }

            uint8_t packet[3] = { on ? 0x90 : 0x80, note, vel };
            tud_midi_stream_write(0, packet, 3);
        }
    }
}

static void core0_loop()
{
    uint64_t next_frame = 0;
    while(1) {
        tud_task();
        cli_run();

        savedata_loop();
        hammer_update();
        proc_midi();
        button_update();
        hid_update();

        cli_fps_count(0);
        sleep_until(next_frame);
        next_frame += 250;
    }
}

/* if certain key pressed when booting, enter update mode */
static void update_check()
{
    const uint8_t pins[] = BUTTON_DEF;
    for (int i = 0; i < count_of(pins); i++) {
        uint8_t gpio = pins[i];
        gpio_init(gpio);
        gpio_set_function(gpio, GPIO_FUNC_SIO);
        gpio_set_dir(gpio, GPIO_IN);
        gpio_pull_up(gpio);
    }

    if (!gpio_get(pins[0]) && !gpio_get(pins[1])) {
        sleep_ms(100);
        reset_usb_boot(0, 2);
        return;
    }
}

void init()
{
    sleep_ms(50);
    board_init();

    update_check();

    set_sys_clock_khz(166000, true);

    tusb_init();
    stdio_init_all();

    config_init();
    mutex_init(&core1_io_lock);
    savedata_init(0xca44caa1, &core1_io_lock);

    light_init();
    hammer_init();
    button_init();

    cli_init("nos_pico>", "\n   << Nos Pico Controller >>\n"
                            " https://github.com/whowechina\n\n");
    
    commands_init();
}

int main(void)
{
    init();
    multicore_launch_core1(core1_loop);
    core0_loop();
    return 0;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen)
{
    printf("Get from USB %d-%d\n", report_id, report_type);
    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize)
{
    if (report_id == REPORT_ID_LIGHTS) {
    }
}
