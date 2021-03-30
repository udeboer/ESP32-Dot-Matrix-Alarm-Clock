/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// clock functions
//
//
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "sdkconfig.h"
#include "string.h"
#include "time.h"

// Own files to include
#include "64bitpatch_localtime.h"
#include "app_queue.h"
#include "defaults_globals.h"
#include "display_clock.h"
#include "display_functions.h"
#include "max7219.h"
#include "nvramfunctions.h"
#include "rotary_encoder.h"
#include "sound.h"
#include "time_task.h"

// Set logging tag per module
static const char* TAG = "ClockDisplay";

// Following var is used by multiple functions
// declaring it here saves ram
static char tempdisp[] = " ";

// We need an extra set of alarm vars. Because pressing sleep
// changes the alarm sounding time from the settings
typedef struct {
  int hour;
  int minute;
  int soundfile_nr;
} next_alarm_t;

next_alarm_t next_alarm = {
    .hour = 99  // If we restart and never get to alarmtime the alarm would go at 00:00
};

typedef struct {
  int sun_up_hour;
  int sun_down_hour;
} sun_updown_t;

// Below struct is for adjusting brightness during the night
// Needs adapting to your location. This is in The Netherlands
// These are average sun up down times for each month. Without
// daylight saving.
const sun_updown_t sun_updown_month[12] = {{9, 17},  // januari is 0 in tm struct
                                           {8, 18}, {7, 18}, {6, 19}, {6, 20}, {5, 21}, {5, 21},
                                           {6, 20}, {6, 19}, {7, 18}, {8, 17}, {9, 17}};

void time_sendto_display(void) {
  // strings are send to the display
  // local time is converted to chars
  max7219_empty_display_buffer();
  // font ascii from 10 tot 19 are clock fonts
  // with fixed font pitch
  tempdisp[0] = ((current_timeinfo.tm_hour / 10) + 10);
  // max7219_sprite_fill_buffer(tempdisp, 7);
  max7219_sprite_fill_buffer(tempdisp, 3);
  tempdisp[0] = ((current_timeinfo.tm_hour % 10) + 10);
  // max7219_sprite_fill_buffer(tempdisp, 13);
  max7219_sprite_fill_buffer(tempdisp, 9);
  // double dot
  tempdisp[0] = 20;
  // max7219_sprite_fill_buffer(tempdisp, 19);
  max7219_sprite_fill_buffer(tempdisp, 15);
  tempdisp[0] = ((current_timeinfo.tm_min / 10) + 10);
  // max7219_sprite_fill_buffer(tempdisp, 22);
  max7219_sprite_fill_buffer(tempdisp, 18);
  tempdisp[0] = ((current_timeinfo.tm_min % 10) + 10);
  // max7219_sprite_fill_buffer(tempdisp, 28);
  max7219_sprite_fill_buffer(tempdisp, 24);
  // display time
  max7219_send_display();
}

void alarm_sendto_display(void) {
  // strings are send to the display
  max7219_empty_display_buffer();
  tempdisp[0] = ((clock_settings.alarmsounds[0].hour / 10) + 10);
  // max7219_sprite_fill_buffer(tempdisp, 2);
  max7219_sprite_fill_buffer(tempdisp, 1);
  tempdisp[0] = ((clock_settings.alarmsounds[0].hour % 10) + 10);
  // max7219_sprite_fill_buffer(tempdisp, 8);
  max7219_sprite_fill_buffer(tempdisp, 7);
  // double dot
  tempdisp[0] = 20;
  // max7219_sprite_fill_buffer(tempdisp, 14);
  max7219_sprite_fill_buffer(tempdisp, 13);
  tempdisp[0] = ((clock_settings.alarmsounds[0].minute / 10) + 10);
  // max7219_sprite_fill_buffer(tempdisp, 17);
  max7219_sprite_fill_buffer(tempdisp, 16);
  tempdisp[0] = ((clock_settings.alarmsounds[0].minute % 10) + 10);
  // max7219_sprite_fill_buffer(tempdisp, 23);
  max7219_sprite_fill_buffer(tempdisp, 22);
  // Alarm fonts chars are 30 and 31. 31 with dash. 30 without
  if (clock_settings.alarm_onoff == true) {
    tempdisp[0] = 31;
  } else {
    tempdisp[0] = 30;
  }
  // max7219_sprite_fill_buffer(tempdisp, 37);
  max7219_sprite_fill_buffer(tempdisp, 28);
  max7219_send_display();
}

void sleep_sendto_display(void) {
  // strings are send to the display
  max7219_empty_display_buffer();
  tempdisp[0] = 'S';
  max7219_sprite_fill_buffer(tempdisp, 0);
  tempdisp[0] = 'l';
  max7219_sprite_fill_buffer(tempdisp, 6);
  tempdisp[0] = 'p';
  max7219_sprite_fill_buffer(tempdisp, 9);
  // Display timeout minutes
  tempdisp[0] = ((clock_settings.sleep_minutes / 10) + 10);
  max7219_sprite_fill_buffer(tempdisp, 21);
  tempdisp[0] = ((clock_settings.sleep_minutes % 10) + 10);
  max7219_sprite_fill_buffer(tempdisp, 27);
  max7219_send_display();
}

void brightness_sendto_display(void) {
  // strings are send to the display
  max7219_empty_display_buffer();
  tempdisp[0] = 'B';
  max7219_sprite_fill_buffer(tempdisp, 0);
  tempdisp[0] = 'r';
  max7219_sprite_fill_buffer(tempdisp, 6);
  tempdisp[0] = 'i';
  max7219_sprite_fill_buffer(tempdisp, 12);
  // Display timeout minutes
  tempdisp[0] = ((clock_settings.brightness / 10) + 10);
  max7219_sprite_fill_buffer(tempdisp, 21);
  tempdisp[0] = ((clock_settings.brightness % 10) + 10);
  max7219_sprite_fill_buffer(tempdisp, 27);
  max7219_send_display();
}

// function to be called with 1 when making settings
// and -1 when called by clock_display clock-tick.
// Called with -1 one or two times a minute to
// delay storing new settings in flash. This protects
// flash from being written too much
void clock_store_nvram(int x) {
  static int count_called;
  if (x < 0) {
    if (count_called < 0) {
      // no settings need to be stored
      return;
    } else {
      // timer is started and we receive -1 to advance counter
      count_called++;
    }
  } else {
    // x is equal or larger than 0. This means start counter
    count_called = 1;
  }
  // Wait 10 ticks. Ticks happen every minute or half minute
  if (count_called > 10) {
    // store clock setting in NVRAM after a delay
    ESP_LOGI(TAG, "Writing clock settings to NVRAM");
    write_nvram(&clock_settings, sizeof(clock_settings_t), NVFLASH_CLOCKBLOB);
    count_called = -255;  // Negative number to stop counter from counting up
  }
}

// Display show alarm. This is called for disable/enable alarm
// When setting the alarm. Also disable sleeping alarm
void alarm_onoff(void) {
  if (clock_settings.alarm_onoff == true) {
    clock_settings.alarm_onoff = false;
    next_alarm.hour = 99;  // Bogus time we never have
  } else {
    clock_settings.alarm_onoff = true;
  }
  clock_store_nvram(1);
}

// Alarm sounded. And we just pressed the switch a long time
// to switch off the alarm
void alarm_off(void) {
  // Disable next alarm. Needed because we always add sleep
  next_alarm.hour = 99;  // Bogus time we never have
  // Option to switch alarm off. Or leave alarm off for next day
  if (!clock_settings.default_on) {
    clock_settings.alarm_onoff = false;
    clock_store_nvram(1);
  }
}

void alarm_add_sleep() {
  next_alarm.minute += clock_settings.sleep_minutes;
  if (next_alarm.minute > 59) {
    next_alarm.minute -= 60;
    next_alarm.hour += 1;
    if (next_alarm.hour > 23) {
      next_alarm.hour = 0;
    }
  }
}

void stop_alarm(void) {
  ESP_LOGI(TAG, "Alarm sound off");
  stop_sound();
}

// Turning the encoder on the alarm display
void alarm_set_time(int adjustment) {
  if (adjustment > 0) {
    // adjust time up
    clock_settings.alarmsounds[0].minute += ALARM_ADJUST;
    if (clock_settings.alarmsounds[0].minute > 59) {
      clock_settings.alarmsounds[0].minute -= 60;
      clock_settings.alarmsounds[0].hour += 1;
      if (clock_settings.alarmsounds[0].hour > 23) {
        clock_settings.alarmsounds[0].hour = 0;
      }
    }
  } else {
    // adjust time down
    clock_settings.alarmsounds[0].minute -= ALARM_ADJUST;
    if (clock_settings.alarmsounds[0].minute < 0) {
      clock_settings.alarmsounds[0].minute += 60;
      clock_settings.alarmsounds[0].hour -= 1;
      if (clock_settings.alarmsounds[0].hour < 0) {
        clock_settings.alarmsounds[0].hour = 23;
      }
    }
  }
  clock_store_nvram(1);
}

// turning the encoder on the set sleep timeout display
void sleep_set_time(int adjustment) {
  if (adjustment > 0) {
    // adjust sleeptime up
    if (clock_settings.sleep_minutes < 59) {
      clock_settings.sleep_minutes += 1;
    }
  } else {
    // adjust time down
    if (clock_settings.sleep_minutes > 1) {
      clock_settings.sleep_minutes -= 1;
    }
  }
  clock_store_nvram(1);
}

//When called with 0 just adjust display brightness to evening or daytime
//When called with positive or negative number adjust brightness setting
void brightness_set(int adjustment) {
  if (adjustment > 0) {
    // adjust brightness up
    if (clock_settings.brightness < MAXBRIGHT) {
      clock_settings.brightness += 1;
      clock_store_nvram(1);
    }
  }
  if (adjustment < 0) {
    // adjust brightness down
    if (clock_settings.brightness > 0) {
      clock_settings.brightness -= 1;
      clock_store_nvram(1);
    }
  }
  // adjust brightness when it is getting darker
  int tmp_hour;
  int tmp_brightness;
  int bright_adjust = clock_settings.brightness / 3;
  if (bright_adjust < 1) {
    bright_adjust = 1;
  }
  tmp_hour = current_timeinfo.tm_hour;
  if (current_timeinfo.tm_isdst) {
    tmp_hour--;
  }
  tmp_brightness = clock_settings.brightness;
  if ((tmp_hour <= (sun_updown_month[current_timeinfo.tm_mon].sun_down_hour + 1)) ||
      (tmp_hour >= (sun_updown_month[current_timeinfo.tm_mon].sun_up_hour - 1))) {
    // We are in the twilight. adjust brightness -1
    tmp_brightness -= bright_adjust;
  }
  if ((tmp_hour <= sun_updown_month[current_timeinfo.tm_mon].sun_down_hour) ||
      (tmp_hour >= sun_updown_month[current_timeinfo.tm_mon].sun_up_hour)) {
    // We are in the dark. adjust brightness -2. So an extra -1.
    tmp_brightness -= bright_adjust;
  }
  // Do not try to set a negative brightness
  if (tmp_brightness < 0) {
    tmp_brightness = 0;
  }
  max7219_set_brightness(tmp_brightness);
}

//Check for sounds or alarms to play
int check_for_alarm(void) {
  // we check if we need to make a sound
  // we have the option to play sounds on certain times
  // return 1 if we sounded the alarm instead of just a sound
  // Alarm will sound looped. Other sounds are not looped
  //
  // We only check and play sounds at the start of the minute.
  // Below is just an extra check on this. Should not be needed.
  if (current_timeinfo.tm_sec > 9) {
    return 0;
  }
  //
  int alarm_found = 0;
  int sound_to_play = -1;
  int x;
  for (x = 0; x < MAX_SOUNDFILES; x++) {
    // loop through the clocksettings and try to find a matching time
    // the first entry in the clock_settings.alarmsound array is the
    // default with the alarm times.
    //////ESP_LOGI(TAG, "X is %d. alarm_found = %d , soundtoplay = %d", x, alarm_found,
    /// sound_to_play);
    if ((current_timeinfo.tm_hour == clock_settings.alarmsounds[x].hour) &&
        (current_timeinfo.tm_min == clock_settings.alarmsounds[x].minute)) {
      // we found a matching time
      ESP_LOGI(TAG, "Slot is %d. alarm_found = %d , soundtoplay = %d", x, alarm_found,
               sound_to_play);
      if (x == 0) {
        // This is the alarm and not another sound.
        alarm_found = 1;
        next_alarm.hour = current_timeinfo.tm_hour;
        next_alarm.minute = current_timeinfo.tm_min;
        next_alarm.soundfile_nr = x;  // This can later be overriden.
        // next_alarm.sleeping = 0;
        ESP_LOGI(TAG, "Check Alarm time found default Alarm.");
      } else {
        ESP_LOGI(TAG, "X achter else is %d.", x);
        if ((alarm_found == 0) && (clock_settings.alarmsounds[x].is_alarm == false)) {
          ESP_LOGI(TAG, "Check Alarm time found default sound.");
          // Found a sound to play and not an alarm. And it is not the alarm time.
          // Checking for certain days of the week
          if ((current_timeinfo.tm_wday + 1) == clock_settings.alarmsounds[x].weekday) {
            // We need to add 1 to tm_wday because it starts at 0. 0 is sunday
            // Found a weekday sound for this hour
            sound_to_play = x;
            ESP_LOGI(TAG, "Check Alarm time found sound for specific weekday.");
          } else if ((clock_settings.alarmsounds[x].month == (current_timeinfo.tm_mon + 1)) &&
                     (clock_settings.alarmsounds[x].day == current_timeinfo.tm_mday)) {
            // just checked if the month, day and time are specific
            sound_to_play = x;
            ESP_LOGI(TAG, "Check Alarm time found sound for specific date.");
          }
          if ((sound_to_play == -1) && (clock_settings.alarmsounds[x].weekday == 0) &&
              (clock_settings.alarmsounds[x].month == 0)) {
            // Sound for certain time. And not for a certain weekday or date
            // We do not allow playing a sound at 00:00 and always.
            if ((clock_settings.alarmsounds[x].hour != 0) &&
                (clock_settings.alarmsounds[x].minute != 0)) {
              sound_to_play = x;
              ESP_LOGI(TAG, "Check Alarm time found sound for certain time and for all days.");
            }
          }
        }
      }
    }
    // Now lets check if there is a special alarm for this day or weekday
    // Walk through all alarm sound and check for weekdays or dates
    // Do not check on time. This allows for special alarm sound for certain
    // days in the week or special dates. Like christmas or weekend days
    if ((alarm_found > 0) && (clock_settings.alarmsounds[x].is_alarm == true)) {
      // check for weekday or day/month
      if ((current_timeinfo.tm_wday + 1) == clock_settings.alarmsounds[x].weekday) {
        // We need to add 1 to tm_wday because it starts at 0. 0 is sunday
        // Found a weekday set
        next_alarm.soundfile_nr = x;
        ESP_LOGI(TAG, "Check Alarm time found alarm for specific weekday.");
      } else if ((clock_settings.alarmsounds[x].month == (current_timeinfo.tm_mon + 1)) &&
                 (clock_settings.alarmsounds[x].day == current_timeinfo.tm_mday)) {
        // just checked if the month, day and time are specific
        next_alarm.soundfile_nr = x;
        ESP_LOGI(TAG, "Check Alarm time found alarm for specific date.");
      }
    }
  }  // End of looping through alarmsounds.
  //
  // now we check if there is an alarm to play
  if ((current_timeinfo.tm_hour == next_alarm.hour) &&
      (current_timeinfo.tm_min == next_alarm.minute) && (clock_settings.alarm_onoff)) {
    ESP_LOGI(TAG, "Alarm is sounding");
    play_wav(clock_settings.alarmsounds[next_alarm.soundfile_nr].soundfile, 1);
    return 1;
  }
  if (sound_to_play > 0) {
    play_wav(clock_settings.alarmsounds[sound_to_play].soundfile, 0);
  }
  return 0;
}

void menu_noop(void) {
  // Sometimes we need to advance display event loop when nothing is in queue
  QueueHandle_t send_queue = display_task_queue();
  const disp_task_queue_item_t signal_to_send = {.disp_task_signal = noop};
  if (send_queue != 0) {
    // we do not check if the queue is full
    xQueueSendToBack(send_queue, &signal_to_send, 0);
  }
}

void menu_timer_queue_send(TimerHandle_t notused) {
  // when the timer expires a signal is send to the display queue.
  // get queue for sending timer end signal
  QueueHandle_t send_queue = display_task_queue();
  // create event to send
  const disp_task_queue_item_t signal_to_send = {.disp_task_signal = timer_expired};
  if (send_queue != 0) {
    // send event
    // we do not check if the queue is full
    xQueueSendToBack(send_queue, &signal_to_send, 0);
  }
}

void menu_timer(u_int timer_timeout) {
  // timeout in milliseconds
  // timeout 0 stops timer
  // timeouts kan be re-armed
  // procedure sends timer_expired to queue
  static TimerHandle_t mtimer_handle = 0;
  // Either start timer or restart it
  if (mtimer_handle == 0) {
    ESP_LOGI(TAG, "Starting menu timer");
    mtimer_handle = xTimerCreate("MTimer", (timer_timeout / portTICK_PERIOD_MS), pdFALSE, 0,
                                 menu_timer_queue_send);
    xTimerStart(mtimer_handle, 5);
  } else {
    ESP_LOGI(TAG, "Restarting menu timer");
    xTimerChangePeriod(mtimer_handle, (timer_timeout / portTICK_PERIOD_MS), 5);
    xTimerReset(mtimer_handle, 5);
  }
}
