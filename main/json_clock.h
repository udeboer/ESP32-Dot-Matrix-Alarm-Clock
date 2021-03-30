/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef JSON_CLOCK_H
#define JSON_CLOCK_H

#include "cJSON.h"

//read and set the clock configuration. JSON <--> clock settings struct. 
//
int json_clock_read(cJSON *receive_json, cJSON *return_json);
int json_clock_set(cJSON *receive_json, cJSON *return_json);

#endif
