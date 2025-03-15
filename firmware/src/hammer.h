/*
 * Piano "Hammer" Action Reading
 * WHowe <github.com/whowechina>
 */

#ifndef HAMMER_H
#define HAMMER_H

void hammer_init();
void hammer_update();
uint8_t hammer_keynum();
uint16_t hammer_velocity(uint8_t chn);
bool hammer_pressed(uint8_t chn);
bool hammer_updated(uint8_t chn);
uint16_t hammer_raw(uint8_t chn);
void hammer_calibrate_origin();
void hammer_calibrate_travel();

#endif
