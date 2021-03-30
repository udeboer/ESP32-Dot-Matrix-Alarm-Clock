/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef SETUP_API_FILES_H_
#define SETUP_API_FILES_H_

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t file_upload_api_post_handler(httpd_req_t *req);
#endif
