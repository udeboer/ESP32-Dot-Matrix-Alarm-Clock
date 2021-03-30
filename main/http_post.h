/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef HTTP_POST_H_
#define HTTP_POST_H_

#include "esp_err.h"
#include "esp_http_server.h"
#include "cJSON.h"
// Own header files

// Function below register the post handlers
void httpd_register_post_uri_handlers(httpd_handle_t server);

// we need the following in multiple files
#define UPLOAD_URI "/api/file_upload"
#endif
