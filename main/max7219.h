/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef MAX7219_H_
#define MAX7219_H_

/* driver for dot matrix displays driven by max7219 chips on
 * spi bus.
 *
 */

#include "driver/spi_master.h"
#include "esp_err.h"
#include <sys/param.h>
#include <sys/types.h>

// define SPI port to be used. Either 2 or 3. Define with SPI2_HOST or SPI3_HOST
#define MAX7219_SPIPORT SPI2_HOST
#define MAX7219_SPISPEED 400000 // SPI baudrate in hz. max 10mhz. Here 400khz. Mhz can give problems on long wires
#define MAX7219_PIN_MOSI 14
#define MAX7219_PIN_CLK 13
#define MAX7219_PIN_CS 15
//
//Rotate 8x8 display 90 degrees clockwise
//Some 4 * 8x8 matrix modules are 90 degrees off. (from china)
//#define MAX7219_ROTATE90 
//
#define MAX7219_COUNT 4   // number of max7219 daisy chained
#define MAX7219_COLUMNS 8 // number of columns driven by 1 max7219.
#define MAX7219_TOT_COLUMNS ( MAX7219_COUNT * MAX7219_COLUMNS )
// Align string to display left, right in the middle
#define MAX7219_ALIGN_LEFT 1
#define MAX7219_ALIGN_RIGHT 2
#define MAX7219_ALIGN_MIDDLE 4
#define MAXBRIGHT 0x0f  // Maximum brightness setting inside max7219 

// Init SPI port
esp_err_t max7219_init_spi(void);

// We have a display buffer with 1 byte for each column of 8 dots
// Send display buffer to display
void max7219_send_display(void);

//Functions to fill or clear display buffer
//get_length of the string you want to send. Used for calculating
//positions in displaybuffer
int max7219_sprite_get_length(char *buf_to_send);

// Clear the display buffer from certain offset
void max7219_sprite_clear_buffer(int offset, uint length);

// Fill the display buffer with certain string. And on certain position
// Position can be (partially)outside of display buffer. Together with clear buffer
// and the length of the string this allows for rolling displays.
void max7219_sprite_fill_buffer(char *buf_to_send, int position);

//Functions below calls above functions.
//And are easier to use 
void max7219_empty_display_buffer(void);
void max7219_fill_display_buffer(char *buf_to_send, int align);

//brightness cannot be set higher than 15 (See datasheet)
void max7219_set_brightness(int brightness);
#endif
