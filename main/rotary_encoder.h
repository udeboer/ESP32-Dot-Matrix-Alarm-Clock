/* Original file modified to use a queue and add some encoder button functionality.
 * This file is GNU opensource. See comments below. Please respect this.
 * Licenced under the GNU GPL Version 3.
 */

/*
 * Copyright (c) 2019 David Antliff
 * Copyright 2011 Ben Buxton
 *
 * This file is part of the esp32-rotary-encoder component.
 *
 * esp32-rotary-encoder is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * esp32-rotary-encoder is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with esp32-rotary-encoder.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ROTARY_ENCODER_H_
#define ROTARY_ENCODER_H_

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "hal/gpio_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Only do full step signals if defined below
// Do half step if commented out
#define FULLSTEP
// Amount of FreeRTOS ticks for debouncing the key press
#define DEBOUNCE_TICKS 8
// Pins used. Is encoder input turning the wrong direction switch pins here.
#define ROT_ENC_A_GPIO GPIO_NUM_33
#define ROT_ENC_B_GPIO GPIO_NUM_32
#define ROT_ENC_PUSH_GPIO GPIO_NUM_27

/**
 * @brief Initialise the rotary encoder device with the specified GPIO pins and full step
 * increments. This function will set up the GPIOs as needed, Note: this function assumes that
 * gpio_install_isr_service(0) has already been called.
 * @param[in, out] info Pointer to allocated rotary encoder info structure.
 * @param[in] pin_a GPIO number for rotary encoder output A.
 * @param[in] pin_b GPIO number for rotary encoder output B.
 * @param[in] pin_switch GPIO number for rotary encoder switch.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t rotary_encoder_init( gpio_num_t pin_a, gpio_num_t pin_b,
                              gpio_num_t pin_switch);

/**
 * Get the state of the push button
 * Button state will be low when pressed. (State is normally pulled high)
 * Used for detecting long key presses.
 */
int rotary_encoder_get_button(void);


#ifdef __cplusplus
}
#endif

#endif  // ROTARY_ENCODER_H
