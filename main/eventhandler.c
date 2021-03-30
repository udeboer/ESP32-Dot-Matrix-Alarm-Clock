/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// This function is here so multiple tasks can start the eventhandler
//
//

#include "eventhandler.h"

//Set logging tag per module
static const char* TAG = "DefaultEvent";

void start_stop_default_event_handler(bool action) {
  // handler_starts counts total processes using the event handler
  static int handler_starts = 0;
  if (action == 1) {
    if (handler_starts == 0) {
      // start event loop
      ESP_LOGI(TAG, "Start default event loop");
      ESP_ERROR_CHECK(esp_event_loop_create_default());
    }
    handler_starts++;
  } else { // action is 0
    if (handler_starts == 1) {
      ESP_LOGI(TAG, "Stop default event loop");
      ESP_ERROR_CHECK(esp_event_loop_delete_default());
    }
    handler_starts--;
    if (handler_starts < 0) {
      ESP_LOGE(TAG, "Error: Event loop stopped while already stopped");
      handler_starts = 0;
    }
  }
}
