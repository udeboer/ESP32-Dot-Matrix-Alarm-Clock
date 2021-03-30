/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// For moving the clock settings struct to and from json.
#include <string.h>
#include <sys/param.h>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h"

// Own header files
#include "defaults_globals.h"
#include "display_functions.h"
#include "http_api_json.h"
#include "max7219.h"  //needed for max brightness

// Set logging tag per module
static const char *TAG = "JsonClockSet";

int json_clock_set(cJSON *receive_json, cJSON *return_json) {
  // First make a copy of clock_settings to store all new
  // settings temporary
  clock_settings_t *temp_settings;
  temp_settings = malloc(sizeof(clock_settings_t));
  memcpy(temp_settings, &clock_settings, sizeof(clock_settings_t));
  // next var is used to count the correct number of good values received
  int return_count = 0;
  // Start parsing the JSON object with the clock settings
  // First var
  cJSON *temp_object = NULL;
  temp_object = cJSON_GetObjectItemCaseSensitive(receive_json, "alarm_on");
  if ((temp_object != NULL) && cJSON_IsBool(temp_object)) {
    // Found JSON object
    if (cJSON_IsTrue(temp_object)) {
      temp_settings->alarm_onoff = true;
    }
    if (cJSON_IsFalse(temp_object)) {
      temp_settings->alarm_onoff = false;
      // If alarm is sounding set it off
      alarm_off();
      stop_alarm();
    }
    return_count++;
  } else {
    ESP_LOGE(TAG, "JSON invalid alarm_on");
  }
  // next-var
  temp_object = NULL;
  temp_object = cJSON_GetObjectItemCaseSensitive(receive_json, "default_on");
  if ((temp_object != NULL) && cJSON_IsBool(temp_object)) {
    // Found JSON object
    if (cJSON_IsTrue(temp_object)) {
      temp_settings->default_on = true;
    }
    if (cJSON_IsFalse(temp_object)) {
      temp_settings->default_on = false;
    }
    return_count++;
  } else {
    ESP_LOGE(TAG, "JSON invalid default_on");
  }
  // next-var
  temp_object = NULL;
  temp_object = cJSON_GetObjectItemCaseSensitive(receive_json, "sleep_minutes");
  if ((temp_object != NULL) && cJSON_IsNumber(temp_object)) {
    // Found JSON object
    if ((temp_object->valueint > 0) && (temp_object->valueint < 61)) {
      temp_settings->sleep_minutes = temp_object->valueint;
      return_count++;
    } else {
      ESP_LOGE(TAG, "JSON invalid sleep_minutes");
    }
  }
  // next-var
  temp_object = NULL;
  temp_object = cJSON_GetObjectItemCaseSensitive(receive_json, "brightness");
  if ((temp_object != NULL) && cJSON_IsNumber(temp_object)) {
    // Found JSON object
    if ((temp_object->valueint >= 0) && (temp_object->valueint <= MAXBRIGHT)) {
      temp_settings->brightness = temp_object->valueint;
      return_count++;
    } else {
      ESP_LOGE(TAG, "JSON invalid brightness");
    }
  }
  // next is the array
  cJSON *alarms = NULL;
  alarms = cJSON_GetObjectItemCaseSensitive(receive_json, "alarms");
  if ((alarms != NULL) && cJSON_IsArray(alarms)) {
    if (cJSON_GetArraySize(alarms) != MAX_SOUNDFILES) {
      ESP_LOGE(TAG, "Clock settings array does not have %d items", MAX_SOUNDFILES);
      ESP_LOGI(TAG, "cJSON Array size is %d", cJSON_GetArraySize(alarms));
      return 400;
    }
    int x = 0;
    cJSON *alarm_item;
    cJSON_ArrayForEach(alarm_item, alarms) {
      temp_object = NULL;
      temp_object = cJSON_GetObjectItemCaseSensitive(alarm_item, "hour");
      if ((temp_object != NULL) && cJSON_IsNumber(temp_object)) {
        // Found JSON object
        if ((temp_object->valueint >= 0) && (temp_object->valueint < 24)) {
          temp_settings->alarmsounds[x].hour = temp_object->valueint;
          return_count++;
        } else {
          ESP_LOGE(TAG, "JSON invalid array item hour");
        }
      }
      temp_object = NULL;
      temp_object = cJSON_GetObjectItemCaseSensitive(alarm_item, "minute");
      if ((temp_object != NULL) && cJSON_IsNumber(temp_object)) {
        // Found JSON object
        if ((temp_object->valueint >= 0) && (temp_object->valueint < 60)) {
          temp_settings->alarmsounds[x].minute = temp_object->valueint;
          return_count++;
        } else {
          ESP_LOGE(TAG, "JSON invalid array item minute");
        }
      }
      temp_object = NULL;
      temp_object = cJSON_GetObjectItemCaseSensitive(alarm_item, "month");
      if ((temp_object != NULL) && cJSON_IsNumber(temp_object)) {
        // Found JSON object
        if ((temp_object->valueint >= 0) && (temp_object->valueint < 13)) {
          temp_settings->alarmsounds[x].month = temp_object->valueint;
          return_count++;
        } else {
          ESP_LOGE(TAG, "JSON invalid array item month");
        }
      }
      temp_object = NULL;
      temp_object = cJSON_GetObjectItemCaseSensitive(alarm_item, "day");
      if ((temp_object != NULL) && cJSON_IsNumber(temp_object)) {
        // Found JSON object
        if ((temp_object->valueint >= 0) && (temp_object->valueint < 32)) {
          temp_settings->alarmsounds[x].day = temp_object->valueint;
          return_count++;
        } else {
          ESP_LOGE(TAG, "JSON invalid array item day");
        }
      }
      temp_object = NULL;
      temp_object = cJSON_GetObjectItemCaseSensitive(alarm_item, "weekday");
      if ((temp_object != NULL) && cJSON_IsNumber(temp_object)) {
        // Found JSON object
        if ((temp_object->valueint >= 0) && (temp_object->valueint < 8)) {
          temp_settings->alarmsounds[x].weekday = temp_object->valueint;
          return_count++;
        } else {
          ESP_LOGE(TAG, "JSON invalid array item weekday");
        }
      }
      temp_object = NULL;
      temp_object = cJSON_GetObjectItemCaseSensitive(alarm_item, "is_alarm");
      if ((temp_object != NULL) && cJSON_IsBool(temp_object)) {
        // Found JSON object
        if (cJSON_IsTrue(temp_object)) {
          temp_settings->alarmsounds[x].is_alarm = true;
        }
        if (cJSON_IsFalse(temp_object)) {
          temp_settings->alarmsounds[x].is_alarm = false;
        }
        return_count++;
      } else {
        ESP_LOGE(TAG, "JSON invalid array item is_alarm");
      }

      temp_object = NULL;
      temp_object = cJSON_GetObjectItemCaseSensitive(alarm_item, "sound");
      if ((temp_object != NULL) && cJSON_IsString(temp_object)) {
        // Found JSON object
        if (strlen(temp_object->string) < MAX_SOUNDFILE_LENGTH) {
          strcpy(temp_settings->alarmsounds[x].soundfile, temp_object->valuestring);
          return_count++;
        } else {
          ESP_LOGE(TAG, "JSON invalid array item sound");
        }
      }

      // advance one array up in the clock settings struct
      x++;
    }
  }

  // We counted the amount of JSON objects returned. Check if correct
  if (return_count == 4 + (7 * MAX_SOUNDFILES)) {
    ESP_LOGI(TAG, "Storing %d returned items ", return_count);
    memcpy(&clock_settings, temp_settings, sizeof(clock_settings_t));
    // store in nvram after a short while
    clock_store_nvram(1);
    free(temp_settings);
    return 0;
  }
  free(temp_settings);
  ESP_LOGI(TAG, "Return items %d do not match", return_count);
  return 400;
}

int json_clock_read(cJSON *receive_json, cJSON *return_json) {
  // we get do not use the received json object. Only
  // put values in the return JSON
  // we read the clock settings into a JSON object
  ESP_LOGI(TAG, "JSON Clock Read settings start");
  if (clock_settings.alarm_onoff) {
    cJSON_AddBoolToObject(return_json, "alarm_on", 1);
  } else {
    cJSON_AddBoolToObject(return_json, "alarm_on", 0);
  }
  if (clock_settings.default_on) {
    cJSON_AddBoolToObject(return_json, "default_on", 1);
  } else {
    cJSON_AddBoolToObject(return_json, "default_on", 0);
  }
  cJSON_AddNumberToObject(return_json, "sleep_minutes", clock_settings.sleep_minutes);
  cJSON_AddNumberToObject(return_json, "brightness", clock_settings.brightness);
  // now add array with alarm times and sounds
  cJSON *alarms = NULL;
  alarms = cJSON_AddArrayToObject(return_json, "alarms");
  if (alarms == NULL) {
    // We have an error in creating array
    ESP_LOGE(TAG, "Error on creating JSON structure. Memory?");
    return 400;
  }
  int x = 0;
  cJSON *alarm_item;
  for (x = 0; x < MAX_SOUNDFILES; x++) {
    // create internal placeholder for alarm object
    alarm_item = NULL;
    alarm_item = cJSON_CreateObject();
    if (alarm_item == NULL) {
      // We have an error in creating JSON structures
      ESP_LOGE(TAG, "Error on creating JSON structure. Memory?");
      return 400;
    }
    cJSON_AddNumberToObject(alarm_item, "hour", clock_settings.alarmsounds[x].hour);
    cJSON_AddNumberToObject(alarm_item, "minute", clock_settings.alarmsounds[x].minute);
    cJSON_AddNumberToObject(alarm_item, "day", clock_settings.alarmsounds[x].day);
    cJSON_AddNumberToObject(alarm_item, "month", clock_settings.alarmsounds[x].month);
    cJSON_AddNumberToObject(alarm_item, "weekday", clock_settings.alarmsounds[x].weekday);
    // next one is a bool
    if (clock_settings.alarmsounds[x].is_alarm) {
      cJSON_AddBoolToObject(alarm_item, "is_alarm", 1);
    } else {
      cJSON_AddBoolToObject(alarm_item, "is_alarm", 0);
    }
    cJSON_AddStringToObject(alarm_item, "sound", clock_settings.alarmsounds[x].soundfile);
    // We created the fields in this array item. Add array item to array
    cJSON_AddItemToArray(alarms, alarm_item);
  }
  return 0;
}
