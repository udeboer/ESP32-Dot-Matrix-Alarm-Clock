/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// HTTPD Post functions
//
//

#include <string.h>
#include <sys/param.h>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "sdkconfig.h"

// Own header files
#include "defaults_globals.h"
#include "eventhandler.h"
#include "filesystem.h"
#include "networkstartstop.h"
#include "nvramfunctions.h"

// Set logging tag per module
static const char *TAG = "JsonNetSetup";

// global and file local global parameters
extern setupparams_t setupparams;

// This handles the setup JSON. Includes all basic setup parameters.
int json_setup_set(cJSON *receive_json, cJSON *return_json) {
  setupparams_t *tempparams;  // for temporary values inside this function
  // return message pointer is overwritten on errors
  char *returnmessage = NULL;
  returnmessage = "Settings Stored";
  int json_parse_error = 0;

  tempparams = malloc(sizeof(setupparams_t));
  // Copy Setup Struct to tmp struct. This should fill the tempory struct with correct values, we
  // now start change.
  memcpy(tempparams, &setupparams, sizeof(setupparams_t));

  // the received JSON struct consist of several object. first copy indiviual items
  // to a seperate object(struct). Then test if settings are strings or numbers and filled.
  // Afterwarts test if they are not to long and have correct values
  cJSON *temp_jsonobject = NULL;
  temp_jsonobject = cJSON_GetObjectItemCaseSensitive(receive_json, "wifimode");
  if (cJSON_IsNumber(temp_jsonobject)) {
    switch (temp_jsonobject->valueint) {
      case 1:
        tempparams->wifimode = 1;
        ESP_LOGI(TAG, "ESP will be AP only");
        break;
      case 2:
        tempparams->wifimode = 2;
        ESP_LOGI(TAG, "ESP will be both AP and Wifi client ");
        break;
      case 3:
        tempparams->wifimode = 3;
        ESP_LOGI(TAG, "ESP will be Wifi client only");
        break;
      case 0:
        tempparams->wifimode = 0;
        ESP_LOGI(TAG, "ESP will switch off wifi");
        break;
      default:
        returnmessage = "Error 10005: Wifi mode not valid";
        json_parse_error = 400;
    }
  } else {
    returnmessage = "Error 10006: wifimode should be a number";
    json_parse_error = 400;
  }

  temp_jsonobject = NULL;
  if (json_parse_error == 0) {
    temp_jsonobject = cJSON_GetObjectItemCaseSensitive(receive_json, "name");
    if (cJSON_GetStringValue(temp_jsonobject) != NULL) {
      if (strlen(cJSON_GetStringValue(temp_jsonobject)) < 32) {
        ESP_LOGI(TAG, "Name is: %s", cJSON_GetStringValue(temp_jsonobject));
        strcpy(tempparams->name, cJSON_GetStringValue(temp_jsonobject));
      } else {
        returnmessage = "Error 10007: ESP Name is too long";
        json_parse_error = 400;
      }
    }
  }

  temp_jsonobject = NULL;
  if (json_parse_error == 0) {
    temp_jsonobject = cJSON_GetObjectItemCaseSensitive(receive_json, "apssid");
    if (cJSON_GetStringValue(temp_jsonobject) != NULL) {
      if (strlen(cJSON_GetStringValue(temp_jsonobject)) <= 32) {
        ESP_LOGI(TAG, "apssid is: %s", cJSON_GetStringValue(temp_jsonobject));
        strcpy(tempparams->apssid, cJSON_GetStringValue(temp_jsonobject));
      } else {
        returnmessage = "Error 10008: AP SSID is too long";
        json_parse_error = 400;
      }
    }
  }

  temp_jsonobject = NULL;
  if (json_parse_error == 0) {
    temp_jsonobject = cJSON_GetObjectItemCaseSensitive(receive_json, "appw");
    if (cJSON_GetStringValue(temp_jsonobject) != NULL) {
      if (strlen(cJSON_GetStringValue(temp_jsonobject)) <= 64) {
        ESP_LOGI(TAG, "appw is: %s", cJSON_GetStringValue(temp_jsonobject));
        strcpy(tempparams->appwd, cJSON_GetStringValue(temp_jsonobject));
      } else {
        returnmessage = "Error 10009: AP key is too long";
        json_parse_error = 400;
      }
    }
  }

  temp_jsonobject = NULL;
  if (json_parse_error == 0) {
    temp_jsonobject = cJSON_GetObjectItemCaseSensitive(receive_json, "apchannel");
    if (cJSON_IsNumber(temp_jsonobject)) {
      if (temp_jsonobject->valueint > 0 && temp_jsonobject->valueint <= 13) {
        ESP_LOGI(TAG, "AP Channel is: %d", temp_jsonobject->valueint);
        tempparams->apchannel = temp_jsonobject->valueint;
      } else {
        returnmessage = "Error 10010: AP Channel not valid 1-13";
        json_parse_error = 400;
      }
    }
  }

  temp_jsonobject = NULL;
  if (json_parse_error == 0) {
    temp_jsonobject = cJSON_GetObjectItemCaseSensitive(receive_json, "stssid");
    if (cJSON_GetStringValue(temp_jsonobject) != NULL) {
      if (strlen(cJSON_GetStringValue(temp_jsonobject)) <= 32) {
        ESP_LOGI(TAG, "stssid is: %s", cJSON_GetStringValue(temp_jsonobject));
        strcpy(tempparams->stssid, cJSON_GetStringValue(temp_jsonobject));
      } else {
        returnmessage = "Error 10011: Wifi client SSID is too long";
        json_parse_error = 400;
      }
    }
  }

  temp_jsonobject = NULL;
  if (json_parse_error == 0) {
    temp_jsonobject = cJSON_GetObjectItemCaseSensitive(receive_json, "stpw");
    if (cJSON_GetStringValue(temp_jsonobject) != NULL) {
      if (strlen(cJSON_GetStringValue(temp_jsonobject)) <= 64) {
        ESP_LOGI(TAG, "stpw is: %s", cJSON_GetStringValue(temp_jsonobject));
        strcpy(tempparams->stpwd, cJSON_GetStringValue(temp_jsonobject));
      } else {
        returnmessage = "Error 10012: AP SSID is too long";
        json_parse_error = 400;
      }
    }
  }

  temp_jsonobject = NULL;
  if (json_parse_error == 0) {
    temp_jsonobject = cJSON_GetObjectItemCaseSensitive(receive_json, "timezone");
    if (cJSON_GetStringValue(temp_jsonobject) != NULL) {
      if (strlen(temp_jsonobject->valuestring) < 40 && strlen(temp_jsonobject->valuestring) > 3) {
        ESP_LOGI(TAG, "timezone is: %s", cJSON_GetStringValue(temp_jsonobject));
        strcpy(tempparams->timezone, cJSON_GetStringValue(temp_jsonobject));
      } else {
        returnmessage = "Error 10013: Timezone string length";
        json_parse_error = 400;
      }
    }
  }

  temp_jsonobject = NULL;
  if (json_parse_error == 0) {
    temp_jsonobject = cJSON_GetObjectItemCaseSensitive(receive_json, "ntpserver");
    if (cJSON_GetStringValue(temp_jsonobject) != NULL) {
      if (strlen(temp_jsonobject->valuestring) < 40 && strlen(temp_jsonobject->valuestring) > 3) {
        ESP_LOGI(TAG, "ntpserver is: %s", cJSON_GetStringValue(temp_jsonobject));
        strcpy(tempparams->ntpserver, cJSON_GetStringValue(temp_jsonobject));
      } else {
        returnmessage = "Error 10014: NTP server name length";
        json_parse_error = 400;
      }
    }
  }

  // Finished JSON parsing.
  ESP_LOGI(TAG, "Finished JSON Parsing");
  // write data to nvram
  if (json_parse_error == 0) {
    ESP_LOGI(TAG, "Storing new values to nvram");
    memcpy(&setupparams, tempparams, sizeof(setupparams_t));
    // Saving struct to nvram
    write_nvram(&setupparams, sizeof(setupparams_t), NVFLASH_SETUPBLOB);
  }
  free(tempparams);  // we do not need this temp struct any more
  ESP_LOGI(TAG, "Back from JSON parser");

  // Create JSON return message to send back
  cJSON_AddStringToObject(return_json, "message", returnmessage);

  if (json_parse_error == 0) {
    // Restart wifi
    // signal Wifi task to change status.
    ESP_LOGI(TAG, "Signal Wifi task");
    wifi_change_event();
  } else {
    ESP_LOGI(TAG, "Not setting up Wifi to new settings");
  }
  return json_parse_error;
}

// return current settings stored inside setup params
int json_setup_read(cJSON *receive_json, cJSON *return_json) {
  int json_parse_error = 0;
  // Create JSON object to send back
  cJSON_AddNumberToObject(return_json, "wifimode", setupparams.wifimode);
  cJSON_AddStringToObject(return_json, "apssid", setupparams.apssid);
  cJSON_AddStringToObject(return_json, "appw", setupparams.appwd);
  cJSON_AddStringToObject(return_json, "stssid", setupparams.stssid);
  cJSON_AddStringToObject(return_json, "stpw", setupparams.stpwd);
  cJSON_AddStringToObject(return_json, "name", setupparams.name);
  cJSON_AddNumberToObject(return_json, "apchannel", setupparams.apchannel);
  cJSON_AddNumberToObject(return_json, "ipmode", setupparams.ipmode);
  cJSON_AddNumberToObject(return_json, "ipbyte1", setupparams.ipbyte1);
  cJSON_AddNumberToObject(return_json, "ipbyte2", setupparams.ipbyte2);
  cJSON_AddNumberToObject(return_json, "ipbyte3", setupparams.ipbyte3);
  cJSON_AddNumberToObject(return_json, "ipbyte4", setupparams.ipbyte4);
  cJSON_AddNumberToObject(return_json, "gwbyte1", setupparams.gwbyte1);
  cJSON_AddNumberToObject(return_json, "gwbyte2", setupparams.gwbyte2);
  cJSON_AddNumberToObject(return_json, "gwbyte3", setupparams.gwbyte3);
  cJSON_AddNumberToObject(return_json, "gwbyte4", setupparams.gwbyte4);
  cJSON_AddNumberToObject(return_json, "maskbyte1", setupparams.maskbyte1);
  cJSON_AddNumberToObject(return_json, "maskbyte2", setupparams.maskbyte2);
  cJSON_AddNumberToObject(return_json, "maskbyte3", setupparams.maskbyte3);
  cJSON_AddNumberToObject(return_json, "maskbyte4", setupparams.maskbyte4);
  cJSON_AddNumberToObject(return_json, "dns1byte1", setupparams.dns1byte1);
  cJSON_AddNumberToObject(return_json, "dns1byte2", setupparams.dns1byte2);
  cJSON_AddNumberToObject(return_json, "dns1byte3", setupparams.dns1byte3);
  cJSON_AddNumberToObject(return_json, "dns1byte4", setupparams.dns1byte4);
  cJSON_AddNumberToObject(return_json, "dns2byte1", setupparams.dns2byte1);
  cJSON_AddNumberToObject(return_json, "dns2byte2", setupparams.dns2byte2);
  cJSON_AddNumberToObject(return_json, "dns2byte3", setupparams.dns2byte3);
  cJSON_AddNumberToObject(return_json, "dns2byte4", setupparams.dns2byte4);
  cJSON_AddStringToObject(return_json, "timezone", setupparams.timezone);
  cJSON_AddStringToObject(return_json, "ntpserver", setupparams.ntpserver);
  return json_parse_error;
}
