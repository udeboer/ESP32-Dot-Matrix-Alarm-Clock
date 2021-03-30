/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef HTTP_GET_H_
#define HTTP_GET_H_

#include "esp_http_server.h"

void httpd_register_get_uri_handlers(httpd_handle_t server);

#endif
