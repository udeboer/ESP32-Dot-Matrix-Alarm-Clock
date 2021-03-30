/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef SOUND_H_
#define SOUND_H_

#define I2S_PORT_NUM I2S_NUM_0 //which ESP32 i2s port to use
#define I2S_BLCK_PIN 5
#define I2S_WS_PIN 18  // other name used for this pin is LRCLK
#define I2S_DATA_OUT_PIN 10

// When using a max98357 i2s amplifier 
#define MAX98357_SD_PIN GPIO_NUM_9 // used to switch off/on DAC. 

void init_i2s(void);
// 0=no repeat, 1=repeat
void play_wav(char *wavsound, int repeat);
void stop_sound(void);

#endif
