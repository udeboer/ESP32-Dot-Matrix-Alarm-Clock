/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// JSON File Listing
//

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h"

// Own header files
#include "defaults_globals.h"
#include "filesystem.h"
#include "http_api_json.h"
#include "json_files.h"

//Set logging tag per module
static const char* TAG = "JsonFiles";

int json_file_list(cJSON *receive_json, cJSON *return_json) {
  // Function is made for SPIFFS without any recursion into sub-dirs
  ESP_LOGI(TAG, "Generate JSON file list");
  int error_to_return = 0;

  // get filesystem name from JSON
  cJSON *fs_name = NULL;
  fs_name = cJSON_GetObjectItemCaseSensitive(receive_json, "filesystem");
  if (!(cJSON_IsString(fs_name) && (fs_name->valuestring != NULL))) {
    ESP_LOGE(TAG, "Error in JSON: filesystem name");
    error_to_return = 400;
  } else {
    ESP_LOGI(TAG, "Directory listing from filesystem %s", fs_name->valuestring);
    // Define filesystem operation structs and vars
    struct dirent *dir_item;
    struct stat file_stats;
    char file_path[MAX_FILEPATH_LENGTH];
    DIR *dir_to_read = opendir(fs_name->valuestring);
    if (dir_to_read == NULL) {
      ESP_LOGE(TAG, "Error in filesystem name in file api");
      error_to_return = 400;
    } else {
      // Create json array and fill with files
      cJSON *dir_json_array = cJSON_AddArrayToObject(return_json, "directory");
      if (dir_json_array == NULL) {
        ESP_LOGE(TAG, "Error in creating json array structures");
        error_to_return = 500;
      } else {
        // Now loop through the directory entries
        while ((dir_item = readdir(dir_to_read)) != NULL) {
          // Dir-item contains 1 file
          // First add path to return string
          strcpy(file_path, fs_name->valuestring);
          // We test if the last character of the path is a slash
          // If not add it.
          if (fs_name->valuestring[strlen(fs_name->valuestring) - 1] != '/') {
            strcat(file_path, "/");
          }
          // Put filename in filenaam string
          strcat(file_path, dir_item->d_name);
          // get stats from file
          stat(file_path,&file_stats);
          ESP_LOGI(TAG, "Filenaam %s", file_path);
          // create a new json object for the file entries.
          cJSON *dir_json_item = cJSON_CreateObject();
          if (dir_json_item == NULL) {
            ESP_LOGE(TAG, "Error in creating json array object");
            error_to_return = 500;
          }
          // add the filename and filesize items to the array object
          if (cJSON_AddStringToObject(dir_json_item, "filename", file_path) == NULL) {
            ESP_LOGE(TAG, "Error in creating json array filename item");
            error_to_return = 500;
          }
          // Add the filesize to the object
          if (cJSON_AddNumberToObject(dir_json_item, "filesize", file_stats.st_size) == NULL) {
            ESP_LOGE(TAG, "Error in creating json array filesize itemt");
            error_to_return = 500;
          }
          // add the object to the array
          cJSON_AddItemToArray(dir_json_array, dir_json_item);
        }
        // Finished while loop.All dir items done
        closedir(dir_to_read);
      }
    }
  }
  ESP_LOGI(TAG, "File list JSON created");
  return error_to_return;
}

// api to delete a file
esp_err_t json_file_delete(cJSON *receive_json, cJSON *return_json) {
  ESP_LOGI(TAG, "Starting JSON delete file");
  int error_to_return = 0;
  // get filename fron JSON
  cJSON *file_to_delete = NULL;
  file_to_delete = cJSON_GetObjectItemCaseSensitive(receive_json, "filename");
  if (cJSON_IsString(file_to_delete) && (file_to_delete->valuestring != NULL)) {
    ESP_LOGI(TAG, "Deleting file %s", file_to_delete->valuestring);
    if (remove(file_to_delete->valuestring) == 0) {
      ESP_LOGI(TAG, "File deleted");
    } else {
      ESP_LOGE(TAG, "Error in deleting file");
      error_to_return = 500;
    }
  } else {
    ESP_LOGE(TAG, "Error in delete file JSON");
    error_to_return = 500;
  }
  ESP_LOGI(TAG, "Back from JSON file delete");
  return error_to_return;
}
