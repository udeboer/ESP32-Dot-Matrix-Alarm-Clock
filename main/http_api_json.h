/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef JSON_API_POST_H_
#define JSON_API_POST_H_

#include "esp_err.h"
#include "esp_http_server.h"
#include "cJSON.h"

//http uri handler for the JSON API
esp_err_t json_api_post_handler(httpd_req_t *req);
#endif
