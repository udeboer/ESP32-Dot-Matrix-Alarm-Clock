/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


/* Module for sending strings to dot matrix display
 * connected with max7219 spi chips.
 */

//#include "driver/gpio.h"
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>

#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/task.h"

// Own file
#include "max7219.h"
// include one of the fonts to your liking
#include "font_terminal.h"
//#include "font_noto.h"

// max7219 registers
#define REGNOOP 0x00
#define REGDIGIT0 0x01
#define REGDIGIT1 0x02
#define REGDIGIT2 0x03
#define REGDIGIT3 0x04
#define REGDIGIT4 0x05
#define REGDIGIT5 0x06
#define REGDIGIT6 0x07
#define REGDIGIT7 0x08
#define REGDECMODE 0x09
#define REGBRIGHT 0x0a
#define REGSCANLIMIT 0x0b
#define REGSHUTDOWN 0x0c
#define REGTEST 0x0f

// Some max7219 register values
#define MODESHUTDOWN 0x00
#define MODENORMAL 0x01
#define MODENOTEST 0x00
#define MODETEST 0x01
#define MODENODECODE 0x00
#define MAXBRIGHT 0x0f
// amount of colums with attached display. See datasheet. Warning.
// Set this to low can damage max7219.
#define SCANLIMIT 0x07  // Means all columns.

//Set logging tag per module
static const char *TAG = "MAX7219";

// function definitions
void max7219_send_buffer(void);
void max7219_send_command(u_int8_t reg_type, u_int8_t reg_value);

const spi_bus_config_t max7219_bus_cfg = {.mosi_io_num = MAX7219_PIN_MOSI,
                                          .miso_io_num = -1,  // not used. Only sending to max7219
                                          .sclk_io_num = MAX7219_PIN_CLK,
                                          .quadwp_io_num = -1,
                                          .quadhd_io_num = -1,
                                          .max_transfer_sz = 0};

const spi_device_interface_config_t max7219_dev_cfg = {.clock_speed_hz = MAX7219_SPISPEED,
                                                       .command_bits = 0,
                                                       .address_bits = 0,
                                                       .dummy_bits = 0,
                                                       .mode = 0,
                                                       .spics_io_num = MAX7219_PIN_CS,
                                                       .duty_cycle_pos = 0,
                                                       .cs_ena_posttrans = 16,
                                                       .cs_ena_pretrans = 16,
                                                       .flags = SPI_DEVICE_NO_DUMMY,
                                                       .pre_cb = NULL,
                                                       .post_cb = NULL,
                                                       .queue_size = 1};

spi_device_handle_t max7219_dev;

// Buffer with data to send to the MAX7219 chips. 
// This buffer is filled from the display buffer for each row
// in the max7219. Or for commands.
// Allocate 2 bytes, 1 voor register. 1 for data
u_int8_t send_buffer[MAX7219_COUNT * 2]; 

// We allocate 1 byte for each column. Byte is a vertical row on
// display.
u_int8_t display_buffer[MAX7219_TOT_COLUMNS];

//Connect the send buffer to an spi transaction
spi_transaction_t max7219_trans = {
    .length = sizeof(send_buffer) * 8, .tx_buffer = &send_buffer, .rx_buffer = NULL};

esp_err_t max7219_init_spi(void) {
  esp_err_t ret;
  ret = spi_bus_initialize(MAX7219_SPIPORT, &max7219_bus_cfg, 0);  // do not use DMA
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "SPI bus for max7219 initalized");
    ret = spi_bus_add_device(MAX7219_SPIPORT, &max7219_dev_cfg, &max7219_dev);
    if (ret == ESP_OK) {
      ESP_LOGI(TAG, "SPI device setup succesfull");
    } else {
      ESP_LOGE(TAG, "Failure in SPI device setup");
    }
  } else {
    ESP_LOGE(TAG, "Failure in SPI bus setup");
  }

  // SPI should be correctly set. Reset max7219
  if (ret == ESP_OK) {
    max7219_send_command(REGSHUTDOWN, MODESHUTDOWN);
    max7219_send_command(REGDIGIT0, 0x55);  // We have dot pattern on display
    max7219_send_command(REGDIGIT1, 0x00);
    max7219_send_command(REGDIGIT2, 0x00);
    max7219_send_command(REGDIGIT3, 0x00);
    max7219_send_command(REGDIGIT4, 0x00);
    max7219_send_command(REGDIGIT5, 0x00);
    max7219_send_command(REGDIGIT6, 0x00);
    max7219_send_command(REGDIGIT7, 0x00);
    max7219_send_command(REGSHUTDOWN, MODESHUTDOWN);
    max7219_send_command(REGTEST, MODENOTEST);
    max7219_send_command(REGSCANLIMIT, SCANLIMIT);
    max7219_send_command(REGDECMODE, MODENODECODE);
    max7219_send_command(REGBRIGHT, 0);
    max7219_send_command(REGSHUTDOWN, MODENORMAL);
  }
  return ret;
}

void max7219_empty_display_buffer(void) {
  // ESP_LOGI(TAG, "Clearing display buffer");
  max7219_sprite_clear_buffer(0, MAX7219_TOT_COLUMNS);
}

// Send 1 line of data to all MAX7219 chips. 1 unique byte for each chip
// to the same register. Send buffer has the unique byte and the register for
// each chip side by side. So daisy chained MAX7219 chips can have there output
// and input connected. See datasheet. 
// Buffer filled with sequences of max7219 register and the value.
// Amount of data is low. No need for DMA
void max7219_send_buffer(void) {
  if (spi_device_polling_transmit(max7219_dev, &max7219_trans) != ESP_OK) {
    ESP_LOGE(TAG, "Error in sending SPI data");
  }
}

// Sometimes it is is nice to know the length of the string to display
// Handy for calculation of placement, blanking, moving the contents
int max7219_sprite_get_length(char *buf_to_send) {
  int tot_chars = strlen(buf_to_send);
  // Determine length of display needed.
  // Add intercharacter spacing after. Except for last char.
  int cols = 0;
  int i;
  uint8_t char_value;
  for (i = 0; i < tot_chars; i++) {
    char_value = (uint8_t)buf_to_send[i];
    if (char_value > 127) {
      ESP_LOGE(TAG, "Only supporting first 127 chars (Ascii) of UTF8");
      return 0;
    } else {
      cols += font_8x8[char_value][0];
      // check if we are on the last character
      // add extra column for intercharater spacing
      if (i < (tot_chars - 1)) {
        cols++;
      }
    }
  }
  return cols;
}

// Clears buffer on given location and length.
// Offset can be negative. Used for fading in the buffer in combination
// with sprite_fill_buffer
void max7219_sprite_clear_buffer(int offset, uint length) {
  // clear display buffer. Counting from 0
  int end;
  // check if we do not cross displaybuffer ends
  end = offset + length;
  if (end > MAX7219_TOT_COLUMNS) {
    end = MAX7219_TOT_COLUMNS;
  }
  if (end < 0) {
    ESP_LOGE(TAG, "Clear buffer parameters wrong");
    return;
  }

  // we now have the correct end (length to clear)
  // Now check start
  if (offset < 0) {
    offset = 0;
  }

  for (; offset < end; offset++) {
    // ESP_LOGI(TAG, "clearing display buffer %d", offset);
    display_buffer[offset] = 0x00;
  }
}

void max7219_sprite_fill_buffer(char *buf_to_send, int position) {
  //ESP_LOGI(TAG, "Start filling Display buffer");
  int tot_chars = strlen(buf_to_send);
  //ESP_LOGI(TAG, "Total characters to send is %d", tot_chars);
  int x;
  int y;
  uint8_t char_value;
  for (x = 0; x < tot_chars; x++) {
    // make the character value an int for font table lookup.
    char_value = (uint8_t)buf_to_send[x];
    // we only have chars of ascii below 128
    if (char_value < 128) {
      // Grep fontbitmap and add to each display buffer line
      // First byte in font char bitmap array is length of char.
      // Start at amount of columns needed for alignment
      for (y = 1; y <= font_8x8[char_value][0]; y++) {
        // testing that we not overrun display buffer
        if ((position >= 0) & (position < MAX7219_TOT_COLUMNS)) {
          display_buffer[position] = font_8x8[char_value][y];
        }
        position++;
      }  // Finished with 1 character
      // add character spacing. This has no effect on last character
      position++;
    } else {
      ESP_LOGE(TAG, "We do not support ASCII greater than 127");
      return;
    }
  }
}

// ascii buffer will be displayed. This is just for easy displaying of text
// Left, Middle or Right alignment of text is possible
void max7219_fill_display_buffer(char *buf_to_send, int align) {
  // ESP_LOGI(TAG, "Sending string to display");
  int cols = 0;
  int shift_cols = 0;  // for calculating the amount of columns to be shifted.
  cols = max7219_sprite_get_length(buf_to_send);
  // we now have the amount of colums to display
  // test for alignment
  if ((align == MAX7219_ALIGN_MIDDLE) & (cols <= MAX7219_TOT_COLUMNS)) {
    shift_cols = (MAX7219_TOT_COLUMNS - cols) / 2;
  }
  if ((align == MAX7219_ALIGN_RIGHT) & (cols <= MAX7219_TOT_COLUMNS)) {
    shift_cols = (MAX7219_TOT_COLUMNS - cols);
  }

  // We now know the length of the string in max7219 display columns.
  // shift_cols contains the shift for middle and right align
  //ESP_LOGI(TAG, "Have %d columns for display", cols);

  // Fill displaybuffer
  max7219_empty_display_buffer();
  max7219_sprite_fill_buffer(buf_to_send, shift_cols);
  max7219_send_display();
}

void max7219_set_brightness(int brightness) {
  if (brightness > MAXBRIGHT) {
    ESP_LOGD(TAG, "Error, brightness cannot be set higher than 15");
    return;
  }
  max7219_send_command(REGBRIGHT, brightness);
}

void max7219_send_command(u_int8_t reg_type, u_int8_t reg_value) {
  int module;
  // ESP_LOGI(TAG, "Send command %d to register %d", reg_value, reg_type);
  for (module = 0; module < MAX7219_COUNT * 2; module += 2) {
    send_buffer[module] = reg_type;
    send_buffer[module + 1] = reg_value;
    max7219_send_buffer();
  }
}

// Here the bitmap from the display buffer is send
// to the display. SPI aquire could be used here.
// to speed up SPI
void max7219_send_display(void) {
  // ESP_LOGI(TAG, "Display buffer is being sent");

#ifdef MAX7219_ROTATE90
  // display buffer needs 8x8 90 degree rotation.
  // Only tested for 8x8 displays
  ////ESP_LOGI(TAG, "Rotate display");
  int chip;
  int bitpos;
  int x;
  u_int8_t bit_to_read = 128;  // set highest bit as starting point
  u_int8_t bit_to_set;
  // bitpos reads each bit in a byte from the displaybuffer
  for (bitpos = 0; bitpos < 8; bitpos++) {
    // For each bit position in displaybuffer array we need
    // to read it and write it to the same max7219
    // register. But to ascending bit positions.
    // bit_to_read is used as mask to read correct bit from byte
    // in displaybuffer.
    for (chip = 0; chip < MAX7219_COUNT; chip++) {
      // Empty sendbuffer bits and set register in buffer
      send_buffer[chip * 2] = bitpos + 1;  // max7219 display register
      send_buffer[chip * 2 + 1] = 0x00;    // clear value here. Less cpu
      // loop over each bit to set in 1 max7219 display register.
      // Rightmost max7219 display needs to be written
      // first in send string.
      bit_to_set = 1;
      for (x = 0; x < 8; x++) {
        if (display_buffer[(((MAX7219_COUNT - 1 - chip) * 8) + x)] & bit_to_read) {
          // we have a 1 value bit in displaybuffer byte and bit
          send_buffer[((chip * 2) + 1)] |= bit_to_set;
        }
        bit_to_set = bit_to_set << 1;
      }
    }
    max7219_send_buffer();  // send each line
    // advance to next bit that needs reading in displaybuffer
    bit_to_read = bit_to_read >> 1;
  }
#endif

#ifndef MAX7219_ROTATE90
  // Display columns do not need rotation
  int col, chip;
  for (col = 0; col < MAX7219_COLUMNS; col++) {
    for (chip = 0; chip < MAX7219_COUNT; chip++) {
      send_buffer[chip * 2] = col + 1;  // which column register to use
      // The last display in the sendbuffer is the left display
      // reverse picking the lines from display buffer
      send_buffer[chip * 2 + 1] =
          display_buffer[((MAX7219_COUNT - 1 - chip) * MAX7219_COLUMNS) + col];  // value
    }
    max7219_send_buffer();  // send each line
  }
#endif
}
