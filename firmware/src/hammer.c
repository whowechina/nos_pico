/*
 * Piano "Hammer" Action Reading
 * WHowe <github.com/whowechina>
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "board_defs.h"

#include "config.h"

#define KEY_NUM 28

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

static uint16_t reading[KEY_NUM];
static uint16_t offset[KEY_NUM];
static uint16_t fscale[KEY_NUM];

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

    for (int i = 0; i < KEY_NUM; i++) {
        uint8_t chn = key_map[i];
        gpio_put(ADC_MUX_A0, chn & 1);
        gpio_put(ADC_MUX_A1, chn & 2);
        gpio_put(ADC_MUX_A2, chn & 4);
        gpio_put(ADC_MUX_A3, chn & 8);
        gpio_put(ADC_MUX_A4, chn & 16);
        reading[i] = read_avg(12);
    }
}

static void proc_signal()
{
    for (int i = 0; i < KEY_NUM; i++) {
        int diff = reading[i] - nos_cfg->baseline[i].released;
        if (nos_cfg->baseline[i].pressed < nos_cfg->baseline[i].released) {
            diff = -diff;
        }
        offset[i] = diff > 0 ? diff : 0;
        fscale[i] = abs(nos_cfg->baseline[i].pressed - nos_cfg->baseline[i].released);
    }
}

void hammer_update()
{
    read_sensors();
    proc_signal();
}

uint8_t hammer_read(uint8_t chn)
{
    return offset[chn] >> 4;
}

bool hammer_down(uint8_t chn)
{
    return offset[chn] >= fscale[chn] / 2;
}

uint8_t hammer_analog(uint8_t chn)
{
    if (fscale[chn] == 0) {
        return 0;
    }

    if (offset[chn] >= fscale[chn]) {
        return 255;
    }

    return (offset[chn] * 255) / fscale[chn];
}

uint16_t hammer_raw(uint8_t chn)
{
    return reading[chn];
}
