/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef WAV_JSON_API_H_
#define WAV_JSON_API_H_

#include "esp_err.h"
#include "esp_http_server.h"
#include "cJSON.h"

// Called by http_api_json to generate list of .wav sound files.
// And return this as JSON. Receive JSON not used here.
int json_wav_list(cJSON *receive_json, cJSON *return_json);
#endif
