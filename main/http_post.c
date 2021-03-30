/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// HTTPD Post functions
// Setup of the handlers and common functions like JSON reading
//

#include "defaults_globals.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"

// own headers
#include "http_api_upload_files.h"
#include "http_api_json.h"
#include "http_post.h"
#include "networkstartstop.h"
#include "webserver.h"

//Set logging tag per module
static const char* TAG = "HttpPost";

// We define the structure for the post handlers here.
// Shows the URL handlers defined for easy reference.
static const httpd_uri_t json_api = {.uri = "/api/json/*",                //
                                      .method = HTTP_POST,                //
                                      .handler = json_api_post_handler,  //
                                      .user_ctx = NULL};
// upload uri is defined in header. Needed in function as well
static const httpd_uri_t file_upload = {.uri = UPLOAD_URI "/*",
                                        .method = HTTP_POST,
                                        .handler = file_upload_api_post_handler,
                                        .user_ctx = NULL};

void httpd_register_post_uri_handlers(httpd_handle_t server) {
  ESP_LOGI(TAG, "Setting up post handlers");
  httpd_register_uri_handler(server, &json_api);
  httpd_register_uri_handler(server, &file_upload);
}

