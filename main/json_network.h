/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef JSON_NETWORK_H_
#define JSON_NETWORK_H_

#include "cJSON.h"

// Returns the current network settings in JSON 
int json_setup_read(cJSON *receive_json, cJSON *return_json);

// Convert the received JSON with the network settings to the 
// setup parameter struct. And if everything is valid restarts
// the network with the new params.
int json_setup_set(cJSON *receive_json, cJSON *return_json);

#endif
