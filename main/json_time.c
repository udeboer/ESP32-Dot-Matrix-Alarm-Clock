/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// For moving the clock settings struct to and from json.
#include <sys/param.h>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h"

// Own header files
#include "defaults_globals.h"
#include "http_api_json.h"
#include "json_time.h"
#include "time_task.h"

// Set logging tag per module
static const char *TAG = "JsonGetSetTime";

int json_time_set(cJSON *receive_json, cJSON *return_json) {
  time_t utc_timestamp = 0;
  // First make a copy of current time to store all new
  // settings temporary
  ESP_LOGI(TAG, "JSON time set");
  // First var
  cJSON *temp_object = NULL;
  temp_object = cJSON_GetObjectItemCaseSensitive(receive_json, "utctimestamp");
  if ((temp_object != NULL) && cJSON_IsNumber(temp_object)) {
    // Found JSON object
    if (temp_object->valuedouble >= 946684800) {  // we test to 1 jan 2000 here
      utc_timestamp = (time_t)temp_object->valuedouble;
    } else {
      ESP_LOGE(TAG, "JSON UTC timestamp should be greater than year 2000");
      return 400;
    }
  }

  if (utc_timestamp == 0) {
    ESP_LOGE(TAG, "JSON UTC timestamp not received");
    return 400;
  }
  set_time(utc_timestamp);
  return 0;
}

int json_time_read(cJSON *receive_json, cJSON *return_json) {
  ESP_LOGI(TAG, "JSON time read");
  time_t epoch_secs = get_time();
  // we get do not use the received json object. Only
  // put values in the return JSON
  cJSON_AddNumberToObject(return_json, "utctimestamp", (double)epoch_secs);
  return 0;
}
