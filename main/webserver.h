/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include <stdbool.h>

#include "esp_err.h"
#include "esp_http_server.h"

// buffer length for all malloc actions for temp buffers.
#define WEB_BUFFER_LENGTH 512  // buffer length for all malloc buffers

// Function declarations

void start_stop_httpd(bool action);
esp_err_t send_http_error(httpd_req_t *req, int req_error);
#endif
