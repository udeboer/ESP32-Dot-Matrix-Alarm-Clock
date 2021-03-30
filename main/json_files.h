/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef FILE_JSON_API_H_
#define FILE_JSON_API_H_

#include "cJSON.h"

//Get files. Spiffs does not have subdirs. Needs filesystem in receive_json
int json_file_list(cJSON *receive_json, cJSON *return_json);

//Delete a file. Needs filenaam in receive_json
int json_file_delete(cJSON *receive_json, cJSON *return_json);
#endif
