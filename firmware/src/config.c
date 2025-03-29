/*
 * Controller Config and Runtime Data
 * WHowe <github.com/whowechina>
 * 
 * Config is a global data structure that stores all the configuration
 * Runtime is something to share between files.
 */

#include "config.h"
#include "savedata.h"

nos_cfg_t *nos_cfg;

static nos_cfg_t default_cfg = {
    .light = {
        .level_key = 64,
        .level_logo = 24,
        .type = 2,
    },
    .hid = {
        .button = false,
        .analog = false,
        .midi = true,
    },
};

nos_runtime_t nos_runtime;

static void config_loaded()
{
}

void config_changed()
{
    savedata_save(false);
}

void config_factory_reset()
{
    *nos_cfg = default_cfg;
    savedata_save(true);
}

void config_init()
{
    nos_cfg = (nos_cfg_t *)savedata_alloc(sizeof(*nos_cfg), &default_cfg, config_loaded);
}
