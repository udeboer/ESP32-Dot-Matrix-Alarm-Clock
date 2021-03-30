/* Original file modified to use a queue and add some encoder button functionality.
 * This file is GNU opensource. See comments below. Please respect this.
 * Licenced under the GNU GPL Version 3. See LICENSE-GPLv3_rotaryEncoder
 */

/*
 * Copyright (c) 2019 David Antliff
 * Copyright 2011 Ben Buxton
 *
 * This file is part of the esp32-rotary-encoder component.
 *
 * esp32-rotary-encoder is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * esp32-rotary-encoder is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with esp32-rotary-encoder.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file rotary_encoder.c
 * @brief Driver implementation for the ESP32-compatible Incremental Rotary Encoder component.
 *
 * Based on https://github.com/buxtronix/arduino/tree/master/libraries/Rotary
 * Original header follows:
 *
 * Rotary encoder handler for arduino. v1.1
 *
 * Copyright 2011 Ben Buxton. Licenced under the GNU GPL Version 3.
 * Contact: bb@cactii.net
 *
 * A typical mechanical rotary encoder emits a two bit gray code
 * on 3 output pins. Every step in the output (often accompanied
 * by a physical 'click') generates a specific sequence of output
 * codes on the pins.
 *
 * There are 3 pins used for the rotary encoding - one common and
 * two 'bit' pins.
 *
 * The following is the typical sequence of code on the output when
 * moving from one step to the next:
 *
 *   Position   Bit1   Bit2
 *   ----------------------
 *     Step1     0      0
 *      1/4      1      0
 *      1/2      1      1
 *      3/4      0      1
 *     Step2     0      0
 *
 * From this table, we can see that when moving from one 'click' to
 * the next, there are 4 changes in the output code.
 *
 * - From an initial 0 - 0, Bit1 goes high, Bit0 stays low.
 * - Then both bits are high, halfway through the step.
 * - Then Bit1 goes low, but Bit2 stays high.
 * - Finally at the end of the step, both bits return to 0.
 *
 * Detecting the direction is easy - the table simply goes in the other
 * direction (read up instead of down).
 *
 * To decode this, we use a simple state machine. Every time the output
 * code changes, it follows state, until finally a full steps worth of
 * code is received (in the correct order). At the final 0-0, it returns
 * a value indicating a step in one direction or the other.
 *
 * It's also possible to use 'half-step' mode. This just emits an event
 * at both the 0-0 and 1-1 positions. This might be useful for some
 * encoders where you want to detect all positions.
 *
 * If an invalid state happens (for example we go from '0-1' straight
 * to '1-0'), the state machine resets to the start until 0-0 and the
 * next valid codes occur.
 *
 * The biggest advantage of using a state machine over other algorithms
 * is that this has inherent debounce built in. Other algorithms emit spurious
 * output with switch bounce, but this one will simply flip between
 * sub-states until the bounce settles, then continue along the state
 * machine.
 * A side effect of debounce is that fast rotations can cause steps to
 * be skipped. By not requiring debounce, fast rotations can be accurately
 * measured.
 * Another advantage is the ability to properly handle bad state, such
 * as due to EMI, etc.
 * It is also a lot simpler than others - a static state table and less
 * than 10 lines of logic.
 */

#include "rotary_encoder.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "hal/gpio_types.h"

// include the queue definitions
#include "app_queue.h"

// Set logging tag per module
static const char *TAG = "RotaryEncoder";

// For debounce on the rotary encoder switch we need a timer

//#define ROTARY_ENCODER_DEBUG
#define TABLE_COLS 4
typedef uint8_t table_row_t[TABLE_COLS];

#define TABLE_ROWS 7

#define DIR_NONE 0x0  // No complete step yet.
#define DIR_CW 0x10   // Clockwise step.
#define DIR_CCW 0x20  // Anti-clockwise step.

#ifndef FULLSTEP
// do half stepping
// Create the half-step state table (emits a code at 00 and 11)
#define R_START 0x0
#define H_CCW_BEGIN 0x1
#define H_CW_BEGIN 0x2
#define H_START_M 0x3
#define H_CW_BEGIN_M 0x4
#define H_CCW_BEGIN_M 0x5

static const uint8_t _ttable_half[TABLE_ROWS][TABLE_COLS] = {
    // 00                  01              10            11                   // BA
    {H_START_M, H_CW_BEGIN, H_CCW_BEGIN, R_START},             // R_START (00)
    {H_START_M | DIR_CCW, R_START, H_CCW_BEGIN, R_START},      // H_CCW_BEGIN
    {H_START_M | DIR_CW, H_CW_BEGIN, R_START, R_START},        // H_CW_BEGIN
    {H_START_M, H_CCW_BEGIN_M, H_CW_BEGIN_M, R_START},         // H_START_M (11)
    {H_START_M, H_START_M, H_CW_BEGIN_M, R_START | DIR_CW},    // H_CW_BEGIN_M
    {H_START_M, H_CCW_BEGIN_M, H_START_M, R_START | DIR_CCW},  // H_CCW_BEGIN_M
};
#endif

#ifdef FULLSTEP
// Create the full-step state table (emits a code at 00 only)
#define R_START 0x0
#define F_CW_FINAL 0x1
#define F_CW_BEGIN 0x2
#define F_CW_NEXT 0x3
#define F_CCW_BEGIN 0x4
#define F_CCW_FINAL 0x5
#define F_CCW_NEXT 0x6

static const uint8_t _ttable_full[TABLE_ROWS][TABLE_COLS] = {
    // 00        01           10           11                  // BA
    {R_START, F_CW_BEGIN, F_CCW_BEGIN, R_START},            // R_START
    {F_CW_NEXT, R_START, F_CW_FINAL, R_START | DIR_CW},     // F_CW_FINAL
    {F_CW_NEXT, F_CW_BEGIN, R_START, R_START},              // F_CW_BEGIN
    {F_CW_NEXT, F_CW_BEGIN, F_CW_FINAL, R_START},           // F_CW_NEXT
    {F_CCW_NEXT, R_START, F_CCW_BEGIN, R_START},            // F_CCW_BEGIN
    {F_CCW_NEXT, F_CCW_FINAL, R_START, R_START | DIR_CCW},  // F_CCW_FINAL
    {F_CCW_NEXT, F_CCW_FINAL, F_CCW_BEGIN, R_START},        // F_CCW_NEXT
};
#endif

/**
 * @brief Struct carries all the information needed by this driver to manage the rotary encoder
 * device. The fields of this structure should not be accessed directly.
 */
typedef struct {
  gpio_num_t pin_a;          // GPIO for Signal A from the rotary encoder device
  gpio_num_t pin_b;          // GPIO for Signal B from the rotary encoder device
  gpio_num_t pin_switch;     // Encoder switch pin. The other side of the switch should be grounded
  QueueHandle_t queue;       // Handle for event queue, created by ::rotary_encoder_create_queue
  const table_row_t *table;  // Pointer to active state transition table
  uint8_t table_state;       // Internal state
  TimerHandle_t keyTimer;    // Timer for switch debounce
} rotary_encoder_info_t;

// make the info var available to this file only
static rotary_encoder_info_t info;

static uint8_t _process() {
  uint8_t event = 0;
  // Get state of input pins.
  uint8_t pin_state = (gpio_get_level(info.pin_b) << 1) | gpio_get_level(info.pin_a);

  // Determine new state from the pins and state table.
#ifdef ROTARY_ENCODER_DEBUG
  uint8_t old_state = info.table_state;
#endif
  info.table_state = info.table[info.table_state & 0xf][pin_state];

  // AND state to only return direction bits from state table
  // Will only return the DIR_CW or DIR_CCW bits and zero for most states
  event = info.table_state & 0x30;
#ifdef ROTARY_ENCODER_DEBUG
  ESP_EARLY_LOGD(TAG, "BA %d%d, state 0x%02x, new state 0x%02x, event 0x%02x", pin_state >> 1,
                 pin_state & 1, old_state, info.table_state, event);
#endif
  return event;
}

static void _isr_rotenc(void *args) {
  // this is a interrupt service routine. Keep it fast and small
  // Create event items
  const disp_task_queue_item_t signal_up = {.disp_task_signal = encoder_up};
  const disp_task_queue_item_t signal_down = {.disp_task_signal = encoder_down};

  uint8_t event = _process();
  BaseType_t task_woken = pdFALSE;
  switch (event) {
    case DIR_CW:
      xQueueSendToBackFromISR(info.queue, &signal_up, &task_woken);
      break;
    case DIR_CCW:
      xQueueSendToBackFromISR(info.queue, &signal_down, &task_woken);
      break;
    default:
      break;
  }
  if (task_woken) {
    portYIELD_FROM_ISR();
  }
}

// For debouncing the switch we start a timer. This timer function checks the state of the pin
// after some time.
static void _isr_rotpush(void *args) {
  // this is a interrupt service routine. Keep it fast and small
  static TickType_t last_tick;
  // Debouncing switch by only looking after X number of FreeRTOS ticks for new events.
  // On first edge we start a timer to sample the state of the input pin after the
  // timer expires. All bouncing edges later are ignored
  TickType_t current_tick;
  current_tick = xTaskGetTickCountFromISR();
  // always 2 freeRTOS ticks longer than the debounce timer delay
  if (current_tick > (last_tick + DEBOUNCE_TICKS + 2)) {
    last_tick = current_tick;
    BaseType_t task_woken = pdFALSE;
    xTimerStartFromISR(info.keyTimer, &task_woken);

    if (task_woken) {
      portYIELD_FROM_ISR();
    }
  }
  // tick timer loop around
  if (current_tick < last_tick) {
    last_tick = current_tick;
  }
}

// Timer expires for key press. This gets called
static void vTimerCallbackKeyPress(xTimerHandle pxTimer) {
  // define the queue signals
  const disp_task_queue_item_t signal_press = {.disp_task_signal = encoder_press};
  const disp_task_queue_item_t signal_press_rel = {.disp_task_signal = encoder_press_released};
  // we should be the debouce time after a key press or release
  // pin is negative when pressed
  if (gpio_get_level(info.pin_switch)) {
    xQueueSendToBack(info.queue, &signal_press_rel, 0);
  } else {
    xQueueSendToBack(info.queue, &signal_press, 0);
  }
}

// For initializing all of the encoder functions
esp_err_t rotary_encoder_init(gpio_num_t pin_a, gpio_num_t pin_b, gpio_num_t pin_switch) {
  esp_err_t err = ESP_OK;
  info.queue = display_task_queue();
  info.pin_a = pin_a;
  info.pin_b = pin_b;
  info.pin_switch = pin_switch;
#ifdef FULLSTEP
  info.table = &_ttable_full[0];  // enable_half_step ? &_ttable_half[0] : &_ttable_full[0];
#endif
#ifndef FULLSTEP
  info.table = &_ttable_half[0];  // enable_half_step ? &_ttable_half[0] : &_ttable_full[0];
#endif
  info.table_state = R_START;

  // Initialize switch debounce timer
  info.keyTimer = xTimerCreate("keyDebounce",   // Name
                               DEBOUNCE_TICKS,  // wait time in ticks
                               pdFALSE,         // One shot timer
                               NULL,                     // Timer id not used
                               vTimerCallbackKeyPress);  // called when timer expires
  if (info.keyTimer == NULL) {
    ESP_LOGE(TAG, "Error in creating KEY press timer");
  }

  // configure GPIOs
  gpio_pad_select_gpio(info.pin_a);
  gpio_set_pull_mode(info.pin_a, GPIO_PULLUP_ONLY);
  gpio_set_direction(info.pin_a, GPIO_MODE_INPUT);
  gpio_set_intr_type(info.pin_a, GPIO_INTR_ANYEDGE);

  gpio_pad_select_gpio(info.pin_b);
  gpio_set_pull_mode(info.pin_b, GPIO_PULLUP_ONLY);
  gpio_set_direction(info.pin_b, GPIO_MODE_INPUT);
  gpio_set_intr_type(info.pin_b, GPIO_INTR_ANYEDGE);

  gpio_pad_select_gpio(info.pin_switch);
  gpio_set_pull_mode(info.pin_switch, GPIO_PULLUP_ONLY);
  gpio_set_direction(info.pin_switch, GPIO_MODE_INPUT);
  gpio_set_intr_type(info.pin_switch, GPIO_INTR_ANYEDGE);
  // install interrupt handlers
  gpio_isr_handler_add(info.pin_a, _isr_rotenc, NULL);
  gpio_isr_handler_add(info.pin_b, _isr_rotenc, NULL);
  gpio_isr_handler_add(info.pin_switch, _isr_rotpush, NULL);
  return err;
}

int rotary_encoder_get_button() {
  return gpio_get_level(info.pin_switch);
}
