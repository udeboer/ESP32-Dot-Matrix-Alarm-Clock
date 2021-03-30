/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 *
 * MIT Licensed as described in the file LICENSE
 */


#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

//Own headers
#include "app_queue.h"

// Returns queue handle and starts queue when not started
// Queue is used to handle user-input and time events. It is the
// main signaling queue for the event driven application.
QueueHandle_t display_task_queue(void) {
  static QueueHandle_t queue_p = 0;
  if (queue_p == 0) {
    queue_p = xQueueCreate(DISP_TASK_QUEUE_LENGTH, sizeof(disp_task_queue_item_t));
  }
  return queue_p;
}
