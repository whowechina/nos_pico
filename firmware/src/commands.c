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

static void handle_calibrate(int argc, char *argv[])
{
    const char *usage = "Usage: calibrate <origin|travel>\n";

    if (argc != 1) {
        printf(usage);
        return;
    }

    const char *choices[] = {"origin", "travel"};
    switch (cli_match_prefix(choices, count_of(choices), argv[0])) {
        case 0:
            hammer_calibrate_origin();
            break;
        case 1:
            hammer_calibrate_travel();
            break;
        default:
            printf(usage);
            break;
    }
}

static void handle_debug(int argc, char *argv[])
{
    const char *usage = "Usage: debug <sensor|velocity>\n";
    if (argc != 1) {
        printf(usage);
        return;
    }
    const char *choices[] = {"sensor", "velocity"};
    switch (cli_match_prefix(choices, count_of(choices), argv[0])) {
        case 0:
            nos_runtime.debug.sensor ^= true;
            break;
        case 1:
            nos_runtime.debug.velocity ^= true;
            break;
        default:
            printf(usage);
            break;
    }
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
    cli_register("debug", handle_debug, "Toggle debug features.");
    cli_register("save", handle_save, "Save config to flash.");
    cli_register("factory", handle_factory_reset, "Reset everything to default.");
}
