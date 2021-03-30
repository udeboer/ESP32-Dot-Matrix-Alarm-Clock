/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef NVRAMFUNCTIONS_H_
#define NVRAMFUNCTIONS_H_
#include "esp_err.h"

//#define NVFLASH_NAMESPACE "storage"  // defined in default_globals.h

//Start NVS 
void init_nvs();

// Read blob from NVRAM. Blob is struct with parameters.
esp_err_t read_nvram(void *blob, size_t sizeof_blob, char *blobname);

// Write blob to NVRAM
void write_nvram(void *blob, size_t sizeof_blob, char *blobname);

#endif
