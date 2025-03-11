/*
 * Piano "Hammer" Action Reading
 * WHowe <github.com/whowechina>
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "board_defs.h"

#include "config.h"

#define KEY_NUM 28

#define PRESS_TRIGGER 500
#define RELEASE_TRIGGER 400

void hammer_init()
{
    gpio_init(ADC_MUX_CS);
    gpio_init(ADC_MUX_WR);
    gpio_init(ADC_MUX_A0);
    gpio_init(ADC_MUX_A1);
    gpio_init(ADC_MUX_A2);
    gpio_init(ADC_MUX_A3);
    gpio_init(ADC_MUX_A4);
    gpio_set_dir(ADC_MUX_CS, GPIO_OUT);
    gpio_set_dir(ADC_MUX_WR, GPIO_OUT);
    gpio_set_dir(ADC_MUX_A0, GPIO_OUT);
    gpio_set_dir(ADC_MUX_A1, GPIO_OUT);
    gpio_set_dir(ADC_MUX_A2, GPIO_OUT);
    gpio_set_dir(ADC_MUX_A3, GPIO_OUT);
    gpio_set_dir(ADC_MUX_A4, GPIO_OUT);
    gpio_put(ADC_MUX_CS, 0);
    gpio_put(ADC_MUX_WR, 0);
    gpio_put(ADC_MUX_A0, 0);
    gpio_put(ADC_MUX_A1, 0);
    gpio_put(ADC_MUX_A2, 0);
    gpio_put(ADC_MUX_A3, 0);
    gpio_put(ADC_MUX_A4, 0);

    adc_init();
    adc_gpio_init(26 + ADC_CHANNEL);
    adc_select_input(ADC_CHANNEL);

    // pwm mode for lower power ripple
    gpio_set_function(25, GPIO_FUNC_PWM);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1);
}

uint8_t hammer_keynum()
{
    return KEY_NUM;
}

static uint16_t reading[KEY_NUM];
uint64_t reading_time;

static uint16_t offset[KEY_NUM];
static uint16_t offset_prev[KEY_NUM];
static uint64_t time_prev;

static uint16_t fscale[KEY_NUM];

uint16_t velocity[KEY_NUM];
bool updated[KEY_NUM];
bool pressed[KEY_NUM];

static inline uint16_t read_avg(int count)
{
    uint32_t sum = 0;
    for (int i = 0; i < count; i++) {
        sum += adc_read();
    }
    return sum / count;
}

static void read_sensors()
{
    static const uint8_t key_map[KEY_NUM] = ADC_KEY_CHN;

    time_prev = reading_time;

    for (int i = 0; i < KEY_NUM; i++) {
        uint8_t chn = key_map[i];
        gpio_put(ADC_MUX_A0, chn & 1);
        gpio_put(ADC_MUX_A1, chn & 2);
        gpio_put(ADC_MUX_A2, chn & 4);
        gpio_put(ADC_MUX_A3, chn & 8);
        gpio_put(ADC_MUX_A4, chn & 16);
        reading[i] = read_avg(12);
    }

    reading_time = time_us_64();
}

static inline void update_velocity(int chn, int delta)
{
    velocity[chn] = delta * 1000 / (reading_time - time_prev);
    updated[chn] = true;
}

static void proc_signal()
{
    for (int i = 0; i < KEY_NUM; i++) {
        fscale[i] = abs(nos_cfg->baseline[i].pressed - nos_cfg->baseline[i].released);

        int diff = reading[i] - nos_cfg->baseline[i].released;
        if (nos_cfg->baseline[i].pressed < nos_cfg->baseline[i].released) {
            diff = -diff;
        }
        if (diff > fscale[i]) {
            diff = fscale[i];
        }
        if (diff < 0) {
            diff = 0;
        }
        offset[i] = diff * 1000 / fscale[i];
        

        if (pressed[i]) {
            if (offset[i] <= RELEASE_TRIGGER) {
                update_velocity(i, offset_prev[i] - offset[i]);
                pressed[i] = false;
            }
        } else {
            if (offset[i] >= PRESS_TRIGGER) {
                update_velocity(i, offset[i] - offset_prev[i]);
                pressed[i] = true;
            }
        }

        offset_prev[i] = offset[i];
    }
}

void hammer_update()
{
    read_sensors();
    proc_signal();
}

uint16_t hammer_velocity(uint8_t chn)
{
    return velocity[chn];
}

bool hammer_pressed(uint8_t chn)
{
    return pressed[chn];
}

bool hammer_updated(uint8_t chn)
{
    if (updated[chn]) {
        updated[chn] = false;
        return true;
    }
    return false;
}

uint16_t hammer_raw(uint8_t chn)
{
    return reading[chn];
}
