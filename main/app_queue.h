/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef APP_QUEUE_H_
#define APP_QUEUE_H_
// Define the queues used inside the application

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// below are all the definitions for 1 queue
#define DISP_TASK_QUEUE_LENGTH 20

//define events to be used
typedef enum {
  // noop (no operations code) needed to keep reading and processing
  noop,
  encoder_up,
  encoder_down,
  encoder_press,
  encoder_press_released,
  minute_passed,
  timer_expired,
} disp_task_signal_t;

// struct is problably overkill.
typedef struct {
  disp_task_signal_t disp_task_signal;
} disp_task_queue_item_t;

// returns the handle for this queue and start it when not already started
QueueHandle_t display_task_queue(void);

#endif
