/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef CLOCK_FUNCTIONS_H_
#define CLOCK_FUNCTIONS_H_

//Various support functions for the main display loop

#include "freertos/FreeRTOS.h"
#include "time.h"
#include <sys/types.h>
#include "freertos/timers.h"

#define MAX_SOUNDFILE_LENGTH 20  // max length of total string for file naam
#define MAX_SOUNDFILES 20        // amount of filenames and date times allowed.
// Below is the name of the NVRAM var with the CLOCK settings
#define NVFLASH_CLOCKBLOB "clock"

// Struct for individual sounds and alarm
typedef struct {
  int hour;
  int minute;
  int month;
  int day;
  int weekday;  // Start on sunday or monday. Have to find out
  char soundfile[MAX_SOUNDFILE_LENGTH];
  bool is_alarm;
} alarmsounds_t;

// Struct for storing Clock settings
typedef struct {
  bool alarm_onoff;
  int sleep_minutes;
  int brightness;
  bool default_on;
  alarmsounds_t
      alarmsounds[MAX_SOUNDFILES];  // room for storing 20 sounds. We use [0] as default alarm
} clock_settings_t;

extern clock_settings_t clock_settings;

// display the time on the display
void time_sendto_display(void);
// display the alarm time and setting on the display
void alarm_sendto_display(void);
// display the sleep timeout for adjusting
void sleep_sendto_display(void);
// display the current brightness setting for adjusting
void brightness_sendto_display(void);
// Store settings to nvram. We do not store the settings straight away
// This function should be called regulary
void clock_store_nvram(int x);
// Switch off/on alarm in setting. When off also disables current alarm
void alarm_onoff(void);
// Switch off current sounding alarm
void alarm_off(void);
// Switch off and at some time to current sounding alarm
void alarm_add_sleep();
// Is called to switch off alarm 
void stop_alarm(void);
// Below three functions adjust the settings
void alarm_set_time(int adjustment);
void sleep_set_time(int adjustment);
void brightness_set(int adjustment);
// function to advance the main loop
void menu_noop(void);
// function called from the timer to send timer event
void menu_timer_queue_send(TimerHandle_t notused);
// start timer 
void menu_timer(u_int timer_timeout);
// called each minute to check for alarm
int check_for_alarm(void);
#endif
