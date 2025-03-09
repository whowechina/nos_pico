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

#define WHITE 0xffffff

static void run_lights()
{
    uint32_t phase = time_us_32() / 50000;
    for (int i = 0; i < 28; i++) {
        light_set_key(i, rgb32_from_hsv(0, 0, hammer_analog(i)), false);
    }
    
    phase = time_us_32() / 10000;
    light_set_logo(rgb32_from_hsv(phase, 255, 255), false);
}

static mutex_t core1_io_lock;
static void core1_loop()
{
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
    uint8_t key[28];
} hid_report = {0};

static void hid_update()
{
    if (tud_hid_ready()) {
        hid_report.buttons = 0;
        for (int i = 0; i < 28; i++){
            hid_report.buttons |= hammer_down(i) ? (1 << i) : 0;
            hid_report.key[i] = 0; //hammer_analog(i);
        }
        tud_hid_n_report(0, REPORT_ID_JOYSTICK, &hid_report, sizeof(hid_report));
    }
}

static void core0_loop()
{
    uint64_t next_frame = 0;
    while(1) {
        tud_task();

        cli_run();

        savedata_loop();
        cli_fps_count(0);

        hid_update();

        hammer_update();

        sleep_until(next_frame);
        next_frame += 1001;
    }
}

/* if certain key pressed when booting, enter update mode */
static void update_check()
{
}

void init()
{
    sleep_ms(50);
    board_init();

    update_check();

    tusb_init();
    stdio_init_all();

    config_init();
    mutex_init(&core1_io_lock);
    savedata_init(0xca44caac, &core1_io_lock);

    light_init();
    hammer_init();

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
