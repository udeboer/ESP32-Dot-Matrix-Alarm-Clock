/**
 * @file ds3231.c
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
 * Lots of functions not needed removed. Some functions adapted to the rest
 * of the program. Uses a simpler i2c function.
 */

#include "ds3231.h"

#include <esp_err.h>

#include "i2c_functions.h"

#define DS3231_STAT_OSCILLATOR 0x80
#define DS3231_STAT_32KHZ 0x08
#define DS3231_STAT_ALARM_2 0x02
#define DS3231_STAT_ALARM_1 0x01

#define DS3231_CTRL_OSCILLATOR 0x80
#define DS3231_CTRL_TEMPCONV 0x20
#define DS3231_CTRL_ALARM_INTS 0x04
#define DS3231_CTRL_ALARM2_INT 0x02
#define DS3231_CTRL_ALARM1_INT 0x01

#define DS3231_ALARM_WDAY 0x40
#define DS3231_ALARM_NOTSET 0x80

#define DS3231_REGSTART_TIME 0x00
#define DS3231_REGSTART_ALARM1 0x07
#define DS3231_REGSTART_ALARM2 0x0b
#define DS3231_REG_CONTROL 0x0e
#define DS3231_REG_STATUS 0x0f
#define DS3231_REG_AGING 0x10
#define DS3231_REGSTART_TEMP 0x11

#define DS3231_12HOUR_FLAG 0x40
#define DS3231_12HOUR_MASK 0x1f
#define DS3231_PM_FLAG 0x20
#define DS3231_MONTH_MASK 0x1f

enum { DS3231_SET = 0, DS3231_CLEAR, DS3231_REPLACE };

static uint8_t bcd2dec(uint8_t val) {
  // From ds3231 bsd registers to decimal
  return (val >> 4) * 10 + (val & 0x0f);
}

static uint8_t dec2bcd(uint8_t val) {
  // To ds3231 bsd register format from decimal notation
  return ((val / 10) << 4) + (val % 10);
}

esp_err_t ds3231_set_time(struct tm *time) {
  const uint8_t reg = DS3231_REGSTART_TIME;
  uint8_t data[7];
  /* time/date data */
  data[0] = dec2bcd(time->tm_sec);
  data[1] = dec2bcd(time->tm_min);
  data[2] = dec2bcd(time->tm_hour);
  /* The week data must be in the range 1 to 7, and to keep the start on the
   * same day as for tm_wday have it start at 1 on Sunday. */
  data[3] = dec2bcd(time->tm_wday + 1);
  data[4] = dec2bcd(time->tm_mday);
  // tm_year is counting from 1900. Clock also. Year highest bit is in month
  // Clock supports from 0 to 199 years. Year 100 = year 2000.
  data[5] = dec2bcd(time->tm_mon + 1);
  if (time->tm_year < 100) {
    data[6] = dec2bcd(time->tm_year);
  } else {
    data[5] += 0x80;  // set highest bit for century
    data[6] = dec2bcd(time->tm_year - 100);
  }
  // write at startreg, one byte , 7 databytes
  i2c_write(DS3231_ADDR, &reg, 1, data, 7);
  return ESP_OK;
}

esp_err_t ds3231_get_time(struct tm *time) {
  const uint8_t reg = DS3231_REGSTART_TIME;
  uint8_t data[7];
  /* read time */
  i2c_read(DS3231_ADDR, &reg, 1, data, 7);
  /* convert to unix time structure */
  time->tm_sec = bcd2dec(data[0]);
  time->tm_min = bcd2dec(data[1]);
  if (data[2] & DS3231_12HOUR_FLAG) {
    /* 12H */
    time->tm_hour = bcd2dec(data[2] & DS3231_12HOUR_MASK) - 1;
    /* AM/PM? */
    if (data[2] & DS3231_PM_FLAG) time->tm_hour += 12;
  } else
    time->tm_hour = bcd2dec(data[2]); /* 24H */
  time->tm_wday = bcd2dec(data[3]) - 1;
  time->tm_mday = bcd2dec(data[4]);
  time->tm_mon = bcd2dec(data[5] & DS3231_MONTH_MASK) - 1;
  // Test for century bit in the month register
  if (data[5] < 0x80) { 
    time->tm_year = bcd2dec(data[6]);
  } else {
    time->tm_year = bcd2dec(data[6]) + 100;
  }
  time->tm_isdst = 0;

  // apply a time zone (if you are not using UTC on the rtc or you want to check/apply DST)
  // applyTZ(time);
  return ESP_OK;
}

esp_err_t ds3231_get_raw_temp(int16_t *temp) {
  const uint8_t reg = DS3231_REGSTART_TEMP;
  uint8_t data[2];

  i2c_read(DS3231_ADDR, &reg, 1, data, sizeof(data));

  *temp = (int16_t)(int8_t)data[0] << 2 | data[1] >> 6;

  return ESP_OK;
}

esp_err_t ds3231_get_temp_integer(int8_t *temp) {
  int16_t t_int;

  esp_err_t res = ds3231_get_raw_temp(&t_int);
  if (res == ESP_OK) *temp = t_int >> 2;

  return res;
}

esp_err_t ds3231_get_temp_float(float *temp) {
  int16_t t_int;

  esp_err_t res = ds3231_get_raw_temp(&t_int);
  if (res == ESP_OK) *temp = t_int * 0.25;

  return res;
}
