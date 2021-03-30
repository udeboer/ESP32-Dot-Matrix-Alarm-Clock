/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// Reading and writing setup blobs for configuration. Into NVRAM
//
//
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"

// Own headers
//
#include "defaults_globals.h"
#include "nvramfunctions.h"

//Set logging tag per module
static const char* TAG = "NVRAM";

// startup nvs flash storage
void init_nvs() {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
}

// read from nvram to blob. Needs pointer to blob. This will be a pointer to a struct
// with parameters. The size of the struct. And the name which is used to 
// store the blob in nvram
esp_err_t read_nvram(void *blob, size_t sizeof_blob, char *blobname) {
  nvs_handle_t setup_handle;
  esp_err_t err = nvs_open(NVFLASH_NAMESPACE, NVS_READONLY, &setup_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error 11001: Problem (%s) opening NVS handle!\n", esp_err_to_name(err));
  } else {
    ESP_LOGI(TAG, "Succesfull in opening NVS voor storage");

    // we opened the NVS now let read the blob.
    esp_err_t err = nvs_get_blob(setup_handle, blobname, blob, &sizeof_blob);
    switch (err) {
      case ESP_OK:
        ESP_LOGI(TAG, "Found %s blob in NVS", blobname);
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGI(TAG, "%s Blob name not found in NVS, could be first time", blobname);
        break;
      default:
        ESP_LOGE(TAG, "Error 11002:(%s) Unknown problem reading NVS parameter",
                 esp_err_to_name(err));
    }
  }
  nvs_close(setup_handle);
  return err;
}

// write struct to nvram
void write_nvram(void *blob, size_t sizeof_blob, char *blobname) {
  nvs_handle_t setup_handle;
  esp_err_t err = nvs_open(NVFLASH_NAMESPACE, NVS_READWRITE, &setup_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error 11003: Problem (%s) opening NVS handle!\n", esp_err_to_name(err));
  } else {
    ESP_LOGI(TAG, "Succesfull in opening NVS voor storage");

    // start writing the struct as one blob to storage.
    if (nvs_set_blob(setup_handle, blobname, blob, sizeof_blob) != ESP_OK) {
      ESP_LOGE(TAG, "Error 11004: writing setup to NVS with %s blob.", blobname);
    }
    nvs_commit(setup_handle);  // write to nvram is only done here
    nvs_close(setup_handle);
  }
}

