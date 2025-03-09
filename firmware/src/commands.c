#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"

#include "config.h"
#include "savedata.h"
#include "cli.h"

#include "hammer.h"

#include "usb_descriptors.h"

static void disp_light()
{
    printf("[Light]\n");
    printf("  Brightness: Key - %d, Logo - %d.\n", nos_cfg->light.level_key,
                                                   nos_cfg->light.level_logo);
}

void handle_display(int argc, char *argv[])
{
    const char *usage = "Usage: display [light]\n";
    if (argc > 1) {
        printf(usage);
        return;
    }

    if (argc == 0) {
        disp_light();
        return;
    }

    const char *choices[] = {"light" };
    switch (cli_match_prefix(choices, count_of(choices), argv[0])) {
        case 0:
            disp_light();
            break;
        default:
            printf(usage);
            break;
    }
}

static int fps[2];
void fps_count(int core)
{
    static uint32_t last[2] = {0};
    static int counter[2] = {0};

    counter[core]++;

    uint32_t now = time_us_32();
    if (now - last[core] < 1000000) {
        return;
    }
    last[core] = now;
    fps[core] = counter[core];
    counter[core] = 0;
}

static void handle_level(int argc, char *argv[])
{
    const char *usage = "Usage: level <key|logo> <0..255>\n";
    if (argc != 2) {
        printf(usage);
        return;
    }

    const char *choices[] = {"key", "logo"};
    int choice = cli_match_prefix(choices, count_of(choices), argv[0]);
    if (choice < 0) {
        printf(usage);
        return;
    }

    int level = cli_extract_non_neg_int(argv[1], 0);
    if ((level < 0) || (level > 255)) {
        printf(usage);
        return;
    }

    if (choice == 0) {
        nos_cfg->light.level_key = level;
    } else {
        nos_cfg->light.level_logo = level;
    }
    
    config_changed();
    disp_light();
}

static void handle_calibrate()
{
    printf("Calibrating key RELEASED...\n");

    uint16_t released[28] = {0};
    uint16_t pressed[28] = {0};

    int avg[28] = {0};
    const int avg_count = 1000;
    for (int i = 0; i < avg_count; i++) {
        hammer_update();
        for (int j = 0; j < 28; j++) {
            avg[j] += hammer_raw(j);
        }
    }
    for (int i = 0; i < 28; i++) {
        released[i] = avg[i] / avg_count;
    }

    printf("Calibrating key PRESSED...\n");
    printf("Please press all keys down, not necessarily simultaneously.\n");

    uint16_t min[28] = {0};
    uint16_t max[28] = {0};
    for (int i = 0; i < 28; i++) {
        min[i] = released[i];
        max[i] = released[i];
    }
    uint64_t stop = time_us_64() + 10000000;
    while (time_us_64() < stop) {
        hammer_update();
        for (int i = 0; i < 28; i++) {
            int val = hammer_raw(i);
            if (val < min[i]) {
                min[i] = val;
            }
            if (val > max[i]) {
                max[i] = val;
            }
        }
    }

    bool success = true;
    for (int i = 0; i < 28; i++) {
        int npole_val = max[i] - released[i];
        int spole_val = released[i] - min[i];
        bool npole = npole_val > 400;
        bool spole = spole_val > 400;
        if (npole != spole) {
            pressed[i] = npole ? max[i] - 50 : min[i] + 50;
            released[i] += npole ? 150 : -150;
        } else {
            printf("Key %d calibration failed. [%d-%d-%d].\n", i, min[i], released[i], max[i]);
            success = false;
            break;
        }
    }

    if (success) {
        for (int i = 0; i < 28; i++) {
            nos_cfg->baseline[i].released = released[i];
            nos_cfg->baseline[i].pressed = pressed[i];
        }
        config_changed();
    }

    for (int i = 0; i < 28; i++) {
        printf("Key %2d: %4d -> %4d, offset: %d.\n", i,
                nos_cfg->baseline[i].released, nos_cfg->baseline[i].pressed,
                abs(nos_cfg->baseline[i].pressed - nos_cfg->baseline[i].released));
    }

    printf("Calibration %s.\n", success ? "succeeded" : "failed");
}

static void handle_save()
{
    savedata_save(true);
}

static void handle_factory_reset()
{
    config_factory_reset();
    printf("Factory reset done.\n");
}

void commands_init()
{
    cli_register("display", handle_display, "Display all config.");
    cli_register("level", handle_level, "Set LED brightness level.");
    cli_register("calibrate", handle_calibrate, "Calibrate the key sensors.");
    cli_register("save", handle_save, "Save config to flash.");
    cli_register("factory", handle_factory_reset, "Reset everything to default.");
}
