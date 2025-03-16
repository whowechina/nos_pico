/*
 * Piano "Hammer" Action Reading
 * WHowe <github.com/whowechina>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "hammer.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "board_defs.h"

#include "config.h"

#define KEY_NUM 28

#define PRESS_TRIGGER 250
#define RELEASE_TRIGGER 300

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
    //gpio_pull_down(26 + ADC_CHANNEL);

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

static uint16_t dist[KEY_NUM];
static uint16_t dist_prev[KEY_NUM];
static uint64_t time_prev;

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
        reading[i] = read_avg(10);
    }

    reading_time = time_us_64();

    if (nos_runtime.debug.sensor) {
        static uint64_t last_print = 0;
        if (reading_time - last_print > 200000) {
            last_print = reading_time;
            printf(":");
            for (int i = 0; i < KEY_NUM; i++) {
                printf(" %4d", reading[i]);
            }
            printf("\n");
        }
    }
}

static inline void update_velocity(int chn, int delta)
{
    velocity[chn] = delta * 1000 / (reading_time - time_prev);
    updated[chn] = true;
}

static void proc_signal()
{
    for (int i = 0; i < KEY_NUM; i++) {
        int offset = abs(reading[i] - nos_cfg->baseline[i].center);
        dist[i] = nos_cfg->baseline[i].magfield * 100 / offset; // in 1/100 mm

        if (pressed[i]) {
            if (dist[i] >= RELEASE_TRIGGER) {
                update_velocity(i, dist[i] - dist_prev[i]);
                pressed[i] = false;
            }
        } else {
            if (dist[i] <= PRESS_TRIGGER) {
                update_velocity(i, dist_prev[i] - dist[i]);
                pressed[i] = true;

                if (nos_runtime.debug.velocity) {
                    printf("Judge offset %4d,", offset);
                    printf(" Distance delta %d-%d,", dist_prev[i], dist[i]);
                    printf(" Velocity %d.\n", dist_prev[i] - dist[i]);
                }
            }
        }

        dist_prev[i] = dist[i];
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

uint8_t hammer_analog(uint8_t chn)
{
    int analog = (dist[chn] - 100) * 255 / 400;
    if (analog < 0) {
        analog = 0;
    }
    if (analog > 255) {
        analog = 255;
    }

    return analog;
}

uint16_t hammer_raw(uint8_t chn)
{
    return reading[chn];
}


static void read_sensors_avg(uint16_t avg[KEY_NUM])
{
    const int avg_count = 1000;
    uint32_t sum[KEY_NUM] = {0};

    for (int i = 0; i < avg_count; i++) {
        read_sensors();
        for (int j = 0; j < KEY_NUM; j++) {
            sum[j] += reading[j];
        }
    }
    for (int i = 0; i < KEY_NUM; i++) {
        avg[i] = sum[i] / avg_count;
    }
}

void hammer_calibrate_origin()
{
    printf("Keep all keys far far away from the sensors.\n");
    printf("Calibrating origin...\n");

    uint16_t origin[KEY_NUM];
    read_sensors_avg(origin);

    printf("Done.\n");
    for (int i = 0; i < KEY_NUM; i++) {
        nos_cfg->sensor.origin[i] = origin[i];
        printf("Key %2d: %4d.\n", i, origin[i]);
    }

    config_changed();
}

void hammer_calibrate_travel()
{
    printf("Calibrating key RELEASED...\n");

    uint16_t released[28] = {0};
    uint16_t pressed[28] = {0};

    read_sensors_avg(released);

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

    printf("Calibration %s.\n", success ? "succeeded" : "failed");

    if (!success) {
        return;
    }

    for (int i = 0; i < KEY_NUM; i++) {
        int press = pressed[i];
        int release = released[i];

        /*
           I'm using this inaccurate (or even wrong) formula to calculate the distance.
           At least it's better than linear interpolation.
           magnetic_strength = magfield (aka. magnetic_coinfiency) / distance
           distance is from the sensor die to the magnet.
           On a Nos Pico, the distance of key pressed = 1mm, key released = 5mm,
           given the magnetic strength (hall sensor readings) of both states,
           we can calculate the center (0 strength) point and the magfield.
           magfield = abs(press - release) * 5 * 1 / 4;
           center = (5 * release - 1 * press) / 4;
           distance = abs(reading - center) * magfield, result is in mm.
        */
        nos_cfg->baseline[i].magfield = abs(press - release) * 5 / 4;
        nos_cfg->baseline[i].center = (5 * release - 1 * press) / 4;
    }
   
    for (int i = 0; i < 28; i++) {
        printf("Key %2d: %4d -> %4d,  magfield: %d, center: %d (%d).\n", i,
                released[i], pressed[i], nos_cfg->baseline[i].magfield, 
                nos_cfg->baseline[i].center, nos_cfg->sensor.origin[i]);
    }

    config_changed();
}