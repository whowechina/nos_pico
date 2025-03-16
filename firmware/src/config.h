/*
 * Controller Config
 * WHowe <github.com/whowechina>
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute__((packed)) {
    struct {
        uint16_t origin[28];
    } sensor;
    struct {
        uint16_t magfield;
        uint16_t center;
    } baseline[28];
    struct {
        uint8_t level_key;
        uint8_t level_logo;
        uint8_t type;
        uint8_t reserved[13];
    } light;
    struct {
        bool button : 1;
        bool analog : 1;
        bool midi : 1;
        uint8_t reserved[15];
    } hid;
} nos_cfg_t;

typedef struct {
    uint16_t fps[2];
    bool key_stuck;
    bool ext_pedal_invert;
    struct {
        bool sensor;
        bool velocity;
    } debug;
} nos_runtime_t;

extern nos_cfg_t *nos_cfg;
extern nos_runtime_t nos_runtime;

void config_init();
void config_changed(); // Notify the config has changed
void config_factory_reset(); // Reset the config to factory default

#endif
