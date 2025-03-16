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
    printf("  Key Light Mode: %s\n", nos_cfg->light.type == 0 ? "Off" :
                           nos_cfg->light.type == 1 ? "Switch" :
                           nos_cfg->light.type == 2 ? "Pressure" : "Velocity");
}

static void disp_hid()
{
    printf("[HID]\n");
    printf("  Button - %s, Analog - %s, MIDI - %s.\n",
           nos_cfg->hid.button ? "On" : "Off",
           nos_cfg->hid.analog ? "On" : "Off",
           nos_cfg->hid.midi ? "On" : "Off");
}

void handle_display(int argc, char *argv[])
{
    const char *usage = "Usage: display [light|hid]\n";
    if (argc > 1) {
        printf(usage);
        return;
    }

    if (argc == 0) {
        disp_light();
        disp_hid();
        return;
    }

    const char *choices[] = {"light", "hid" };
    switch (cli_match_prefix(choices, count_of(choices), argv[0])) {
        case 0:
            disp_light();
            break;
        case 1:
            disp_hid();
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

static void handle_hid(int argc, char *argv[])
{
    const char *usage = "Usage: hid <button|analog|midi> [on|off]\n";
    if (argc < 1) {
        printf(usage);
        return;
    }
    const char *choices[] = {"button", "analog", "midi"};
    int choice = cli_match_prefix(choices, count_of(choices), argv[0]);

    int on = 1;
    if (argc == 2) {
        const char *on_off[] = {"off", "on"};
        on = cli_match_prefix(on_off, count_of(on_off), argv[1]);
    } else if (argc > 2) {
        on = -1;
    }

    if ((choice < 0) || (on < 0)) {
        printf(usage);
        return;
    }

    switch (choice) {
        case 0:
            nos_cfg->hid.button = on;
            break;
        case 1:
            nos_cfg->hid.analog = on;
            break;
        case 2:
            nos_cfg->hid.midi = on;
            break;
    }

    config_changed();
    disp_hid();
}

static void handle_light(int argc, char *argv[])
{
    const char *usage = "Usage: light <off|switch|pressure|velocity>\n";
    if (argc != 1) {
        printf(usage);
        return;
    }
    const char *choices[] = {"off", "switch", "pressure", "velocity"};
    int select = cli_match_prefix(choices, count_of(choices), argv[0]);
    if (select < 0) {
        printf(usage);
        return;
    }

    nos_cfg->light.type = select;
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
    cli_register("light", handle_light, "Set light mode.");
    cli_register("hid", handle_hid, "Set hid report types.");
    cli_register("calibrate", handle_calibrate, "Calibrate the key sensors.");
    cli_register("debug", handle_debug, "Toggle debug features.");
    cli_register("save", handle_save, "Save config to flash.");
    cli_register("factory", handle_factory_reset, "Reset everything to default.");
}
