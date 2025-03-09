/*
 * Piano "Hammer" Action Reading
 * WHowe <github.com/whowechina>
 */

#ifndef HAMMER_H
#define HAMMER_H

void hammer_init();
void hammer_update();
uint8_t hammer_read(uint8_t chn);
bool hammer_down(uint8_t chn);
uint8_t hammer_analog(uint8_t chn);
uint16_t hammer_raw(uint8_t chn);

#endif
