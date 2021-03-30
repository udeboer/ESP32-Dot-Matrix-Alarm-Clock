/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// Webserver functions
//
//
#include <string.h>
#include <sys/param.h>
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h"

// Own header files
#include "defaults_globals.h"
#include "http_get.h"
#include "http_post.h"
#include "networkstartstop.h"
#include "webserver.h"

// Set logging tag per module
static const char* TAG = "WebServer";

static httpd_handle_t start_webserver(void);
static void stop_webserver(httpd_handle_t server);

//Simple wrapper for starting and stopping the webserver
//No need to rember the webserver httpd_handle
void start_stop_httpd(bool action) {
  static httpd_handle_t server = NULL;
  static bool httpd_started = 0;  // Function can be called multiple times.
  if ((action == 1) && (httpd_started == 0)) {
    ESP_LOGI(TAG, "Start Webserver");
    server = start_webserver();
    httpd_started = 1;
  }
  if ((action == 0) && (httpd_started == 1)) {
    ESP_LOGI(TAG, "Stop Webserver");
    stop_webserver(server);
    httpd_started = 0;
  }
}

//Generic function for returning HTTP errors
esp_err_t send_http_error(httpd_req_t* req, int req_error) {
  // Routine to send error return on http request
  // Should be called from within URI handler
  ESP_LOGI(TAG, "Now sending error for error number %d.", req_error);
  switch (req_error) {
    case 400:
      return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Badly formed request");
      break;
    case 404:
      return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Web page not found");
      break;
    case 405:
      return httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "Parameters not allowed");
      break;
    case 414:
      return httpd_resp_send_err(req, HTTPD_414_URI_TOO_LONG, "Request too long");
      break;
    case 500:
      return httpd_resp_send_500(req);
      break;
    default:
      return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Something went wrong with the request");
  }
}

static httpd_handle_t start_webserver(void) {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  // Change to a different function for matching the URI.
  // Default only does a strcmp. So no wildcards possible.
  // This is the default IDF provided URI wildcard matching function.
  config.uri_match_fn = httpd_uri_match_wildcard;

  // Start the httpd server
  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    // register uri handlers in their own files
    httpd_register_get_uri_handlers(server);
    httpd_register_post_uri_handlers(server);
    return server;
  }

  ESP_LOGE(TAG, "Error starting server!");
  return NULL;
}

static void stop_webserver(httpd_handle_t server) {
  // Stop the httpd server
  ESP_LOGI(TAG, "Going to stop httpd");
  httpd_stop(server);
  ESP_LOGI(TAG, "Stopped httpd");
}
