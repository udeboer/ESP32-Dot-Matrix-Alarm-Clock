/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// Webpage functions
//

#include <string.h>
#include <sys/param.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h"

// Own header files
#include "defaults_globals.h"
#include "filesystem.h"
#include "http_get.h"
#include "webserver.h"

// Set logging tag per module
static const char *TAG = "HttpGet";

// Default HTTP Get handler services files from spiffs
static esp_err_t default_files_get_handler(httpd_req_t *req) {
  // this is sending the files.
  // Only support for ASCII Urls and filenames. And not for subdirectories
  // And no support for spaces, parameters, fragments. So no % in URL
  // Url's with no dot will have .html added to their name.
  ESP_LOGI(TAG, "This is the URI %s", req->uri);
  size_t uri_length = strlen(req->uri);
  ESP_LOGI(TAG, "This is the URI length %d. ", uri_length);

  int request_error = 0;
  // const size_t max_uri_length = MAXVFSPATHLENGTH - FILESYSTEM1_BASE_SIZE;
  // ESP_LOGI(TAG, "This is the max URI length %d. ", max_uri_length);
  // if (uri_length > MAXVFSPATHLENGTH-FILESYSTEM1_BASE_SIZE ) {
  if (uri_length > MAXVFSPATHLENGTH - FILESYSTEM1_BASE_SIZE) {
    // if (uri_length > max_uri_length ) {
    request_error = 414;
  } else {
    // Testing on single characters not allowed. and other stuff
    if (uri_length > 1) {
      if (strchr(req->uri + 1, '/') != NULL) {
        // no extra slashes in url. No subdirs on SPIFFS
        request_error = 400;
      }
    }
    if (req->uri[0] != '/') {
      // First character should be a /
      request_error = 400;
    }

    if (strchr(req->uri, '?') != NULL) {
      // No parameters allowed
      request_error = 405;
    }
    if (strchr(req->uri, '#') != NULL) {
      // No url fragments
      request_error = 400;
    }
    if (strchr(req->uri, '%') != NULL) {
      // Only ascii. No spaces etc..
      request_error = 400;
    }
    if ((strchr(req->uri, '.') == NULL) &&
        (uri_length > MAXVFSPATHLENGTH - 5 - FILESYSTEM1_BASE_SIZE)) {
      // No dot in url. So we problably need to add .html
      // Testing if this is not larger than max file length
      request_error = 414;
    }
    if (strchr(req->uri, ' ') != NULL) {
      // No spaces in Url. This should not happen. spaces normally converted to %20
      request_error = 400;
    }
  }

  // A correct url should have been received.
  //
  if (request_error == 0) {
    char filename_to_serve[MAXVFSPATHLENGTH + 1];
    // first add spiffs filesytem path
    strcpy(filename_to_serve, FILESYSTEM1_BASE);
    if (uri_length == 1) {
      // We only have a / . So filename to return is index.html
      strcat(filename_to_serve, "/index.html");
    } else {
      strcat(filename_to_serve, req->uri);
      if (strchr(req->uri, '.') == NULL) {
        strcat(filename_to_serve, ".html");
      }
    }
    // test if file exists.
    ESP_LOGI(TAG, "This file %s should be send", filename_to_serve);
    FILE *file_d = NULL;
    file_d = fopen(filename_to_serve, "r");
    if (!file_d) {
      // file could not be opened. file_d is 0 with a ! makes true.
      // Done for more logical layout.
      ESP_LOGW(TAG, "This file %s could not be found", filename_to_serve);
      request_error = 404;
    } else {
      // we have a file. Set the correct header
      // First get file extension from URI
      //
      char file_ext[9];
      if (strlen(strchr(filename_to_serve, '.')) >= 8) {
        ESP_LOGE(TAG, "Error URI extension to long.");
      } else {
        strcpy(file_ext, strchr(filename_to_serve, '.'));
        ESP_LOGI(TAG, "File extension to serve is %s.", file_ext);
      }
      // Other content types should be added when needed.
      if (!strcmp(file_ext, ".html")) {
        ESP_LOGI(TAG, "Sending text/html application type");
        httpd_resp_set_type(req, "text/html");
      } else if (!strcmp(file_ext, ".css")) {
        ESP_LOGI(TAG, "Sending text/css application type");
        httpd_resp_set_type(req, "text/css");
      } else if (!strcmp(file_ext, ".js")) {
        ESP_LOGI(TAG, "Sending text/javasctipt application type");
        httpd_resp_set_type(req, "text/javascript");
      } else if (!strcmp(file_ext, ".jpg")) {
        ESP_LOGI(TAG, "Sending image/jpeg application type");
        httpd_resp_set_type(req, "image/jpeg");
      } else if (!strcmp(file_ext, ".svg")) {
        ESP_LOGI(TAG, "Sending image/svg application type");
        httpd_resp_set_type(req, "image/svg+xml");
      } else if (!strcmp(file_ext, ".png")) {
        ESP_LOGI(TAG, "Sending image/png application type");
        httpd_resp_set_type(req, "image/png");
      } else if (!strcmp(file_ext, ".ico")) {
        ESP_LOGI(TAG, "Sending image/x-icon application type");
        httpd_resp_set_type(req, "image/x-icon");
      } else {
        // default content type. Browsers will start save as dialog.
        ESP_LOGI(TAG, "Sending default application type");
        httpd_resp_set_type(req, "application/octet-stream");
      }

      // We send the file.
      char *sendbuf;
      sendbuf = malloc(WEB_BUFFER_LENGTH);
      size_t fileread_size;
      do {
        // readfiles in temp buffer
        fileread_size = fread(sendbuf, 1, WEB_BUFFER_LENGTH, file_d);
        // ESP_LOGI(TAG, "File part size read = %d bytes.", (int)fileread_size);
        if (fileread_size > 0) {
          // send the buffers received as chunks.
          if (httpd_resp_send_chunk(req, sendbuf, fileread_size) != ESP_OK) {
            // For some reason we could not send the file chunk
            ESP_LOGE(TAG, "Error sending file chunk.");
            fclose(file_d);
            free(sendbuf);
            return ESP_FAIL;
          }
          // ESP_LOGI(TAG, "End of sending file part.");
        }
        // As long as we can read contents from file we send. If there is nothing to read
        // fread will return 0
      } while (fileread_size > 0);
      // ESP_LOGI(TAG, "End of sending total file.");
      // close the file
      fclose(file_d);
      free(sendbuf);
      // ESP_LOGI(TAG, "End of cleaning file structures.");
    }
  }

  if (request_error == 0) {
    // set header so http connection closes.
    httpd_resp_set_hdr(req, "Connection", "close");
    // sending empty chunk to signal we are finished
    httpd_resp_send_chunk(req, NULL, 0);
    ESP_LOGI(TAG, "File send ok.");
    return ESP_OK;
  } else {
    ESP_LOGI(TAG, "Error in url processing is %d.", request_error);
    if (send_http_error(req, request_error) != ESP_OK) {
      ESP_LOGE(TAG, "Error sending error status to client.");
      return ESP_FAIL;
    }
  }
  return ESP_OK;
}

/* Simple GET handler to return hello. copied from examples. Handy for testing*/
static esp_err_t hello_get_handler(httpd_req_t *req) {
  /* Send response with custom headers and body set as the
   * string passed in user context*/
  const char *resp_str = (const char *)req->user_ctx;
  httpd_resp_send(req, resp_str, strlen(resp_str));

  /* After sending the HTTP response the old HTTP request
   * headers are lost. Check if HTTP request headers can be read now. */
  if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
    ESP_LOGI(TAG, "Request headers lost");
  }
  return ESP_OK;
}

// register the functions above as html get handlers
static const httpd_uri_t default_files = {
    .uri = "/*", .method = HTTP_GET, .handler = default_files_get_handler, .user_ctx = NULL};

static const httpd_uri_t hello = {.uri = "/hello/?*",
                                  .method = HTTP_GET,
                                  .handler = hello_get_handler,
                                  /* Let's pass response string in user
                                   * context to demonstrate it's usage */
                                  .user_ctx = "Hello World!"};

void httpd_register_get_uri_handlers(httpd_handle_t server) {
  httpd_register_uri_handler(server, &hello);
  httpd_register_uri_handler(server, &default_files);
}
