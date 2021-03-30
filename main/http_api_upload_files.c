/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// HTTPD Post functions
//
//

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "sdkconfig.h"

// Own header files
#include "defaults_globals.h"
#include "filesystem.h"
#include "http_api_upload_files.h"
#include "http_post.h"
#include "webserver.h"

// Set logging tag per module
static const char *TAG = "HttpFileUpload";

esp_err_t file_upload_api_post_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "Starting http post file ");
  int request_error = 0;
  struct stat file_stat;
  const char *file_path;
  char header_received[30];

  // Check for the content-type application/octet-stream
  if ((httpd_req_get_hdr_value_str(req, "Content-Type", header_received,
                                   (sizeof(header_received) - 1))) == ESP_OK) {
    // read content type header
    if (strcmp("application/octet-stream", header_received) == 0) {
      ESP_LOGI(TAG, "File uploaded with the correct header");
    } else {
      ESP_LOGE(TAG, "File uploaded with wrong Content-Type header value.");
      request_error = 400;
    }
  } else {
      ESP_LOGE(TAG, "File uploaded without Content-Type header.");
      request_error = 400;
  }

  if (request_error == 0) {
    // We use the end of the uri as file name
    if ((strlen(req->uri) - strlen(UPLOAD_URI)) > MAX_FILEPATH_LENGTH) {
      request_error = 414;
      ESP_LOGE(TAG, "File upload path to long");
    }
  }

  if (request_error == 0) {
    ESP_LOGI(TAG, "File upload uri is: %s", req->uri);
    // file name to store. Including path
    file_path = (req->uri + strlen(UPLOAD_URI));
    ESP_LOGI(TAG, "File path is %s", file_path);
    // checking if file exists. Remove it
    if (stat(file_path, &file_stat) == 0) {
      ESP_LOGI(TAG, "File already exists. Remove it");
      unlink(file_path);
    }

    // Need to put in a bunch of testing for existing file and filesystem
    FILE *file_to_write = NULL;
    int bytes_todo = req->content_len;
    int bytes_received;
    char *read_buffer = NULL;
    // allocate buffer and open file te write
    read_buffer = malloc(WEB_BUFFER_LENGTH);
    file_to_write = fopen(file_path, "w");

    // we loop through the request body and write the file
    while (bytes_todo > 0) {
      ESP_LOGI(TAG, "Bytes todo : %d", bytes_todo);
      bytes_received = httpd_req_recv(req, read_buffer, MIN(bytes_todo, WEB_BUFFER_LENGTH));
      // error checking
      if (bytes_received <= 0) {
        if (bytes_received == HTTPD_SOCK_ERR_TIMEOUT) {
          continue;  // only error where we can try again
        } else {
          // Unknown problem close file. remove file and stop receive loop
          ESP_LOGE(TAG, "Error in receiving file. Connection closed");
          bytes_todo = -1;
          request_error = 500;
        }
      }

      // No problems reading http input
      // Start writing the file and check if size written equals bytes received.
      if (bytes_received == fwrite(read_buffer, 1, bytes_received, file_to_write)) {
        ESP_LOGI(TAG, "%d bytes written from %d bytes to do.", bytes_received, bytes_todo);
        bytes_todo -= bytes_received;
      } else {
        ESP_LOGE(TAG, "Error writing buffer to file");
        bytes_todo = -1;
        request_error = 500;
      }
    }
    // Finished reading file. Close file
    fclose(file_to_write);
    if (bytes_todo < 0) {
      unlink(file_path);
    }
    free(read_buffer);
  }

  if (request_error == 0) {
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_sendstr(req, "File uploaded");
    return ESP_OK;
  } else {
    // request went wrong send error
    send_http_error(req, request_error);
    return ESP_FAIL;
  }
}
