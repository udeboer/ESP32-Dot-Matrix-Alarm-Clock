/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// Generate a JSON with wav names without directory and .wav
//

#include "cJSON.h"
#include "esp_err.h"
#include "esp_system.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "sdkconfig.h"

// Own header files
#include "defaults_globals.h"
#include "filesystem.h"
#include "http_api_json.h"
#include "json_wavs.h"

//Set logging tag per module
static const char* TAG = "JsonGetWavs";

esp_err_t json_wav_list(cJSON *receive_json, cJSON *return_json) {
  ESP_LOGI(TAG, "Generate wav listing from filesystem %s", FILESYSTEM1_BASE);
  int error_to_return = 0;
  // Define filesystem operation structs and vars
  struct dirent *dir_item;
  char wav_item[MAX_FILEPATH_LENGTH];
  DIR *dir_to_read = opendir(FILESYSTEM1_BASE);
  if (dir_to_read == NULL) {
    ESP_LOGE(TAG, "Error opening directory for .wav files");
    error_to_return = 400;
  } else {
    // read wavs into json. First create JSON array
    cJSON *wav_json_array = cJSON_AddArrayToObject(return_json, "wavs");
    if (wav_json_array == NULL) {
      ESP_LOGE(TAG, "Error in creating json array structures");
      error_to_return = 500;
    } else {
      // Now we need to loop through the directory entries
      while ((dir_item = readdir(dir_to_read)) != NULL) {
        // dir-item contains 1 file or directory. Copy to string
        // because we want to edit it.
        strcpy(wav_item,dir_item->d_name);
        //Get pointer to .
        char *wav_point = strstr(wav_item, ".wav");
        if (wav_point != NULL) {
          // Got a .wav file in wav_item and the dot in wav_point
          // replace dot with strng terminator
          *wav_point = '\0';
          ESP_LOGI(TAG, "Wav short name %s", wav_item);
          // create a new json object for short wav name.
          cJSON *wav_json_item = cJSON_CreateObject();
          if (wav_json_item == NULL) {
            ESP_LOGE(TAG, "Error in creating json array object");
            error_to_return = 500;
          }
          // add the wav short name
          if (cJSON_AddStringToObject(wav_json_item, "wavsound", wav_item) == NULL) {
            ESP_LOGE(TAG, "Error in creating json array wav item");
            error_to_return = 500;
          }
          // add the object to the array
          cJSON_AddItemToArray(wav_json_array, wav_json_item);
        }
      }
      // Finished while loop.All dir items done
      closedir(dir_to_read);
    }
  }
  ESP_LOGI(TAG, "Wav list JSON created");
  return error_to_return;
}
