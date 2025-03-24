/*
 * Nos Controller Board Definitions
 * WHowe <github.com/whowechina>
 */

#if defined BOARD_NOS_PICO

#define RGB_PIN_KEY 2
#define RGB_PIN_LOGO 0
#define RGB_ORDER GRB // or RGB

#define ADC_MUX_CS 17
#define ADC_MUX_WR 16
#define ADC_MUX_A0 22
#define ADC_MUX_A1 21
#define ADC_MUX_A2 20
#define ADC_MUX_A3 19
#define ADC_MUX_A4 18

#define ADC_CHANNEL 0

#define ADC_KEY_CHN { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, \
                     27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16 }

#define BUTTON_DEF { 13, 12, 10, 11 }
#else

#endif
