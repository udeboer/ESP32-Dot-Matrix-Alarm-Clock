/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// This file generates a queue event just after passing the 30 and 0 second mark
// And includes the start of sntp and other functions
//

#include <sys/select.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/apps/sntp.h"
#include "sdkconfig.h"
#include "sntp.h"
#include "time.h"

// Own files to include
// 64bitpath_localtime.h is here to workaround 64bit time_t bugs in newlib
// All xxx_patch functions are not 64bit time_t in newlib (Toolchain and IDF version v4.2 03-2020)
#include "64bitpatch_localtime.h"
#include "app_queue.h"
#include "defaults_globals.h"
#include "ds3231.h"
#include "nvramfunctions.h"
#include "time_task.h"

// Set logging tag per module
static const char *TAG = "Time";

// Struct is also used outside of this file. Do not use seconds.
// Seconds are only updated 2 times a minute.
struct tm current_timeinfo;

// Below is started as a FreeRTOS task. And send events just after passing the minute mark. And just
// after passing the 30 seconds mark.
void systemtime_start() {
  ESP_LOGI(TAG, "Time will be started");
  static struct timeval now;
  static struct tm ds3231_tm;
  // On startup read time from realtime clock. Realtime clock has UTC time.
  // Needs UTC as timezone
  setenv("TZ", "", 1);
  tzset_patch();
  ds3231_get_time(&ds3231_tm);
  now.tv_sec = mktime(&ds3231_tm);
  settimeofday(&now, NULL);

  // get queue for sending second updates
  static QueueHandle_t send_queue;
  send_queue = display_task_queue();
  //Create event
  static disp_task_queue_item_t signal_to_send;
  signal_to_send.disp_task_signal = minute_passed;
  // Set Timezone
  // setenv("TZ", "CET-1CES-2,M3.5.0/2,M10.5.0/3", 1);
  setenv("TZ", setupparams.timezone, 1);
  // initialise for first run in while loop
  tzset_patch();
  gettimeofday(&now, NULL);
  localtime_patch(&now.tv_sec, &current_timeinfo);

  // The loop below is trying to generate a queue entry just after the zero and
  // 30 second passing. The vtaskdelay is timed to expire just after the start of second 0
  // or 30.
  while (1) {
    if (current_timeinfo.tm_sec < 30) {
      // Delay just over the 30 or 60 second mark. 1050000 below to be sure of just passing second
      // switchover. We use current seconds from tm and microseconds from the timeval structure. And
      // calculate how long to wait.
      vTaskDelay(
          ((((1050000 - (int32_t)now.tv_usec) / 1000) + ((29 - current_timeinfo.tm_sec) * 1000)) /
           portTICK_PERIOD_MS));
    } else {
      vTaskDelay(
          ((((1050000 - (int32_t)now.tv_usec) / 1000) + ((59 - current_timeinfo.tm_sec) * 1000)) /
           portTICK_PERIOD_MS));
    }
    // we should now be just after second switch (or 30 sec).
    setenv("TZ", setupparams.timezone, 1);
    tzset_patch();
    gettimeofday(&now, NULL);
    // current_timeinfo is used elsewhere. Seconds are not correct. Because not updated every second.
    localtime_patch(&now.tv_sec, &current_timeinfo);
    // Sending second signal on queue
    if (send_queue != 0) {
      // we do not check if the queue is full
      xQueueSendToBack(send_queue, &signal_to_send, 0);
    }
    // SNTP update elapsed minutes since last update
    // Below also updates the counter
    // When we just had a sync (once a hour) copy time to realtime clock
    if (last_sntp_sync_min() == 1) {
      // write sntp time to realtime clock
      gettimeofday(&now, NULL);
      if (ds3231_set_time(gmtime(&now.tv_sec)) == ESP_OK) {
        ESP_LOGI(TAG, "Succesful set time in DS3231");
      } else {
        ESP_LOGE(TAG, "Error in pushing time to DS3231");
      }
    }
    // when we do not have an sntp sync for a long time. Use ds3231 to
    // set time. Internal clock of ESP32 is not very stable
    if ((last_sntp_sync_min() > 0) && ((last_sntp_sync_min() % SNTP_MAX_TIME_NOSYNC) == 0)) {
      // Read time from realtimeclock
      // Needs UTC as timezone
      setenv("TZ", "", 1);
      tzset_patch();
      ds3231_get_time(&ds3231_tm);
      now.tv_sec = mktime(&ds3231_tm);
      settimeofday(&now, NULL);
      ESP_LOGV(TAG, "No recent SNTP sync. DS3231 sets time.");
    }
  }
}

//wrapper for easy starting the timetask
void start_time_task() {
  ESP_LOGI(TAG, "Start Time task");
  // other tasks are all started with lower prio of 4
  xTaskCreate(&systemtime_start, "TimeTask", 4096, NULL, 5, NULL);
}

void start_stop_sntp(bool action) {
  if (action == 1) {
    ESP_LOGI(TAG, "SNTP will be started");
    // Below 3lines are needed to prevent asserts
    if (sntp_enabled()) {
      sntp_stop();
    }
    ESP_LOGI(TAG, "SNTP settings done now");
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, setupparams.ntpserver);
    sntp_init();
  } else {
    ESP_LOGI(TAG, "SNTP will be stopped");
    sntp_stop();
  }
}

// Function to count, check for and store the last time sntp did an update. 
// Function should be called at least ones a minute to advance counter.
int last_sntp_sync_min() {
  static int last_min;
  static int count;
  if (last_min != current_timeinfo.tm_min) {
    last_min = current_timeinfo.tm_min;
    count++;
  }
  switch (sntp_get_sync_status()) {
    case SNTP_SYNC_STATUS_COMPLETED:
      ESP_LOGI(TAG, "SNTP sync complete");
      count = 0;
      break;
    case SNTP_SYNC_STATUS_IN_PROGRESS:
      ESP_LOGI(TAG, "SNTP smooth sync in progress");
      break;
    case SNTP_SYNC_STATUS_RESET:
      // ESP_LOGE(TAG, "SNTP sync in progress or not done");
      break;
    default:
      ESP_LOGE(TAG, "SNTP sync unknown status");
  }
  return count;
}

// Returns UTC Time
time_t get_time() {
  ESP_LOGI(TAG, "Get time_t now");
  struct timeval now;
  //////////tzset_patch();
  gettimeofday(&now, NULL);
  return (time_t)now.tv_sec;
}

// Use UTC time
void set_time(time_t new_time) {
  struct timeval now;
  gettimeofday(&now, NULL);
  ESP_LOGI(TAG, "Old ESP32 timestamp %lld", now.tv_sec);
  ESP_LOGI(TAG, "New ESP32 timestamp %lld", new_time);
  now.tv_sec = new_time;
  settimeofday(&now, NULL);
  if (ds3231_set_time(gmtime(&now.tv_sec)) == ESP_OK) {
    ESP_LOGI(TAG, "Succesful manual set time in DS3231");
  } else {
    ESP_LOGE(TAG, "Error in pushing time to DS3231");
  }
}
