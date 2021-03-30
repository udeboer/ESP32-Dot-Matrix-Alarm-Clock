/**
 * @file ds3231.h
 * @defgroup ds3231 ds3231
 * @{
 *
 * ESP-IDF driver for DS337 RTC and DS3231 high precision RTC module
 *
 * Ported from esp-open-rtos
 *
 * Copyright (C) 2015 Richard A Burton <richardaburton@gmail.com>\n
 * Copyright (C) 2016 Bhuvanchandra DV <bhuvanchandra.dv@gmail.com>\n
 * Copyright (C) 2018 Ruslan V. Uss <unclerus@gmail.com>
 *
 * MIT Licensed as described in the file LICENSE-ds3231
 */

/* @Modified by Udo de Boer 2021
 * This file is adapted from above. LICENSE file renamed to LICENSE-ds3231
 * lot of functions not needed removed
 */

#ifndef __DS3231_H__
#define __DS3231_H__

#include <esp_err.h>
#include <stdbool.h>
#include <time.h>

#include "i2c_functions.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DS3231_ADDR 0x68  //!< I2C address

/**
 * @brief Set the time on the RTC
 *
 * Timezone agnostic, pass whatever you like.
 * I suggest using GMT and applying timezone and DST when read back.
 *
 * @return ESP_OK to indicate success
 */
esp_err_t ds3231_set_time(struct tm *time);

/**
 * @brief Get the time from the RTC, populates a supplied tm struct
 *
 * @param[out] time RTC time
 * @return ESP_OK to indicate success
 */
esp_err_t ds3231_get_time(struct tm *time);

/**
 * @brief Get the raw temperature value
 *
 * **Supported only by DS3231**
 *
 * @param[out] temp Raw temperature value
 * @return ESP_OK to indicate success
 */
esp_err_t ds3231_get_raw_temp(int16_t *temp);

/**
 * @brief Get the temperature as an integer
 *
 * **Supported only by DS3231**
 *
 * @param[out] temp Temperature, degrees Celsius
 * @return ESP_OK to indicate success
 */
esp_err_t ds3231_get_temp_integer(int8_t *temp);

/**
 * @brief Get the temperature as a float
 *
 * **Supported only by DS3231**
 *
 * @param[out] temp Temperature, degrees Celsius
 * @return ESP_OK to indicate success
 */
esp_err_t ds3231_get_temp_float(float *temp);

#ifdef __cplusplus
}
#endif

/**@}*/

#endif /* __DS3231_H__ */
