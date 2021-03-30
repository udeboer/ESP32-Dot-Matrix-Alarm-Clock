/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// HTTPD Post functions
//
//

#include <string.h>
#include <sys/param.h>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h"

// Own header files
#include "defaults_globals.h"
#include "http_api_json.h"
#include "http_post.h"
#include "json_clock.h"
#include "json_files.h"
#include "json_network.h"
#include "json_wavs.h"
#include "json_time.h"
#include "webserver.h"

//Set logging tag per module
static const char* TAG = "HttpToJson";

// return the adres of a pointer to a cJSON object filled with the http request.
// this way because a free (malloc) must be possible outside this function
esp_err_t http_request_json_parse(httpd_req_t *req, cJSON **cjson_parsed) {
  ESP_LOGI(TAG, "Starting to put req into cJSON");
  char *readbuf;
  int result = 0;
  int reqsize = req->content_len;
  // this is here to avoid buffer overflows.
  if (reqsize <= JSONREADBUFSIZE) {
    ESP_LOGI(TAG, "Chars to read %d", reqsize);
    readbuf = malloc(reqsize + 1);
  } else {
    ESP_LOGE(TAG, "Error. JSON request too long");
    send_http_error(req, 414);
    return ESP_FAIL;
  }
  while (result <= 0) {
    // Read the data for the request
    if ((result = httpd_req_recv(req, readbuf, reqsize)) <= 0) {
      if (result == HTTPD_SOCK_ERR_TIMEOUT) {
        // Retry receiving if timeout occurred
        ESP_LOGE(TAG, "Error. Receive timeout.");
        continue;
      }
      if ( (result == HTTPD_SOCK_ERR_FAIL) || (result == HTTPD_SOCK_ERR_INVALID )){
        ESP_LOGE(TAG, "Error. Receive unknown Socket Error.");
      }
      free(readbuf);
      // Something stange happened
      send_http_error(req, 415);
      return ESP_FAIL;
    }
    // Data received
    ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
    ESP_LOGI(TAG, "%.*s", result, readbuf);
    ESP_LOGI(TAG, "====================================");
  }
  // Add null to make the buffer a string
  readbuf[reqsize] = '\0';

  // Parse JSON
  ESP_LOGI(TAG, "JSON parser with JSON  %s", readbuf);
  // parse the jsonstring to an cJSON structure
  *cjson_parsed = cJSON_Parse(readbuf);
  free(readbuf);  // free the http message buffer. The data is inside JSON structures
  if (cjson_parsed == NULL) {
    const char *cjson_error_ptr = cJSON_GetErrorPtr();
    if (cjson_error_ptr != NULL) {
      ESP_LOGE(TAG, "Error before: %s", cjson_error_ptr);
    }
    // error handling todo
    send_http_error(req, 405);
    return ESP_FAIL;
  }
  return ESP_OK;
}


// Below we handle the JSON request. First read the string in the http body
// into a cJSON object. Then we check if there is a valid JSON command in it.
// We call the function for that command.
esp_err_t json_api_post_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "Entering HTTP POST on /api/json");
  // All request for data go through the request api.
  if (strcmp(req->uri, "/api/json/request") != 0) {
    // We did not found a valid url to react on.
    ESP_LOGE(TAG, "HTTP no /api/json handler for %s", req->uri);
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "HTTP POST on /api/json/request");

  // Checks for correct URL finished
  cJSON *receive_json = NULL;
  // here receive_json gets memory. Do not forget cJSON_Delete when finished
  if (http_request_json_parse(req, &receive_json) == ESP_FAIL) {
    // error in reading request to parsed json
    return ESP_FAIL;
  }
  // We have some form of valid JSON
  // Create return vars
  int error_to_return = 0;
  cJSON *return_json = NULL;
  // here return_json gets memory. Do not forget cJSON_Delete when finished
  return_json = cJSON_CreateObject();
  if (return_json == NULL) {
    ESP_LOGE(TAG, "Error in creating return JSON object");
    return ESP_FAIL;
  }

  // Create JSON objects to fill with RequestType command
  cJSON *request_type = NULL;
  request_type = cJSON_GetObjectItemCaseSensitive(receive_json, "RequestType");

  if (cJSON_IsString(request_type)) {
    // Inside the JSON is a string for RequestType
    // Now call the correct function with both the received json
    // and the JSON object to return. The request JSON can have more
    // fields specific for the type of request. But will not be changed.

    error_to_return = -1;  // This should be overwritten below

    // Setup settings read
    if (strcmp(request_type->valuestring, "SetupRead") == 0) {
      ESP_LOGI(TAG, "HTTP POST request SetupRead");
      error_to_return = json_setup_read(receive_json, return_json);
    }
    // Setup settings set
    if (strcmp(request_type->valuestring, "SetupSet") == 0) {
      ESP_LOGI(TAG, "HTTP POST request SetupSet");
      error_to_return = json_setup_set(receive_json, return_json);
    }

    // Clock settings read
    if (strcmp(request_type->valuestring, "ClockRead") == 0) {
      ESP_LOGI(TAG, "HTTP POST request ClockRead");
      error_to_return = json_clock_read(receive_json, return_json);
    }
    // Clock settings set
    if (strcmp(request_type->valuestring, "ClockSet") == 0) {
      ESP_LOGI(TAG, "HTTP POST request ClockSet");
      error_to_return = json_clock_set(receive_json, return_json);
    }
    
    // Time read
    if (strcmp(request_type->valuestring, "TimeRead") == 0) {
      ESP_LOGI(TAG, "HTTP POST request TimeRead");
      error_to_return = json_time_read(receive_json, return_json);
    }
    // Time set
    if (strcmp(request_type->valuestring, "TimeSet") == 0) {
      ESP_LOGI(TAG, "HTTP POST request TimeSet");
      error_to_return = json_time_set(receive_json, return_json);
    }

    // Directory listing
    if (strcmp(request_type->valuestring, "FileList") == 0) {
      ESP_LOGI(TAG, "HTTP POST request FileList");
      error_to_return = json_file_list(receive_json, return_json);
    }

    // Wav listing
    if (strcmp(request_type->valuestring, "WavList") == 0) {
      ESP_LOGI(TAG, "HTTP POST request WavList");
      error_to_return = json_wav_list(receive_json, return_json);
    }

    // Single file delete
    if (strcmp(request_type->valuestring, "DeleteFile") == 0) {
      ESP_LOGI(TAG, "HTTP POST request DeleteFile");
      error_to_return = json_file_delete(receive_json, return_json);
    }

    // If error to return is still -1 than no valid subroutine is found
    if (error_to_return == -1) {
      error_to_return = 400;
      ESP_LOGE(TAG, "HTTP POST No valid RequestType received");
    }
  } else {
    // JSON for Request Type not correct
    ESP_LOGE(TAG, "HTTP POST RequestType not part of JSON");
    error_to_return = 400;
  }
  // Now start sending back resulting JSON or error
  // Release allocated memory
  if (error_to_return == 0) {
    // should not forget to free the var httpbuf. Memory is allocated here
    char *httpbuf = cJSON_PrintUnformatted(return_json);
    ESP_LOGI(TAG, "Back from JSON building. JSON = %s", httpbuf);
    // Now we need to send a responce back. Add headers
    httpd_resp_set_hdr(req,"Connection","close");
    httpd_resp_set_type(req, "application/json");
    if (httpd_resp_send(req, httpbuf, strlen(httpbuf)) != ESP_OK){
        ESP_LOGE(TAG, "Httpd error in sending correct JSON.");
    }
    free(httpbuf);
  } else {
    // if we got an error we send it here
    send_http_error(req, error_to_return);
  }
  // Finished JSON parsing.
  ESP_LOGI(TAG, "Back from JSON parser");
  // cleanup the temporary json structs
  if (return_json != NULL) {
    cJSON_Delete(return_json);
  }
  if (receive_json != NULL) {
    cJSON_Delete(receive_json);
  }
  // return the correct error code
  if (error_to_return == 0) {
    return ESP_OK;
  } else {
    return ESP_FAIL;
  }
  // End of JSON Request
}

