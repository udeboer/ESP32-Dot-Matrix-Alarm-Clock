/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef JSON_TIME_H
#define JSON_TIME_H

#include "cJSON.h"

// Called from http_api_json. Gets or sets UTC time from/to ESP32
// JSON object is utctimestamp.
int json_time_read(cJSON *receive_json, cJSON *return_json);
int json_time_set(cJSON *receive_json, cJSON *return_json);

#endif
