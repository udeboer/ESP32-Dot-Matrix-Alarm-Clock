/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// Display and encoder
//
#include <sys/types.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"
#include "string.h"

// Own files to include
#include "app_queue.h"
#include "defaults_globals.h"
#include "display_clock.h"
#include "display_functions.h"
#include "max7219.h"
#include "networkstartstop.h"
#include "nvramfunctions.h"
#include "rotary_encoder.h"
#include "time_task.h"

// Set logging tag per module
static const char* TAG = "ClockDisplay";

//What is currently to be shown on the display and
//some special states.
enum display_state_t {
  clockdisplay,
  alarmdisplay,
  alarmset,
  alarmgoing,
  alarmgoingpressed,
  alarmgoingoff,
  sleeptime,
  brightness,
  openwifiap,
  openwifiappressed
} display_state;

//Clock_sttings holds all the clock settings(not networking) and is stored
//in NVRAM after not being changed for a while. 
clock_settings_t clock_settings = {
    // some default values
    .alarm_onoff = false,
    .brightness = 0,
    .sleep_minutes = 5,
    .default_on = 1,
    .alarmsounds[0] = {.hour = 10,
                       .minute = 15,
                       .month = 0,
                       .day = 0,
                       .weekday = 0,
                       .is_alarm = 1,
                       .soundfile = "bird1"}};

//This is the main task for the clock display. Waits for events. Processes
//them and loops to wait for new events. It is started as a FreeRTOS task.
void display_clock() {
  // this function will never return. Declare variables static
  ESP_LOGI(TAG, "Start display and buttons");
  // starting display
  ESP_LOGI(TAG, "Reseting SPI and MAX7219");
  max7219_init_spi();
  max7219_set_brightness(2);
  max7219_fill_display_buffer("Clock", MAX7219_ALIGN_MIDDLE);
  max7219_send_display();

  if (read_nvram(&clock_settings, sizeof(clock_settings_t), NVFLASH_CLOCKBLOB) != ESP_OK) {
    ESP_LOGE(TAG, "No clock settings found in NVRAM. Using default.");
  }

  // create queue and get handle
  static QueueHandle_t disp_queue;
  disp_queue = display_task_queue();

  // Start encoder and its interrupt routines
  ESP_ERROR_CHECK(gpio_install_isr_service(0));
  ESP_ERROR_CHECK(rotary_encoder_init(ROT_ENC_A_GPIO, ROT_ENC_B_GPIO, ROT_ENC_PUSH_GPIO));

  // create var for receiving queue signals
  static disp_task_queue_item_t queue_item;

  // On boot, display clock
  display_state = clockdisplay;
  // Put something in queue to start clock display sooner on startup
  // Also initialize timer
  ESP_LOGI(TAG, "Waiting 4000 ms to start clock display");
  menu_timer(4000);

  // Endless loop that receives events and displays everything.
  // Every start of a minute an event is generated. Or other events like
  // keypresses.
  // The display is only updated after receiving an event.
  while (1) {
    // start waiting on queue events
    while (xQueueReceive(disp_queue, &queue_item, 70000 / portTICK_PERIOD_MS) != pdTRUE) {
      // we have waited for 70 seconds on items in queue. Should get 1 every 30secs
      ESP_LOGE(TAG, "Error: No display events received for 70 seconds");
    }
    // we now should have a valid event type in queue_item
    switch (queue_item.disp_task_signal) {
      // What happens on an encoder switch press. Where do we go next.
      case encoder_press:
        ESP_LOGI(TAG, "Encoder Press event received");
        // from current to new display state
        switch (display_state) {
          case clockdisplay:
            ESP_LOGI(TAG, "Switching from clock to alarm display");
            display_state = alarmdisplay;
            menu_timer(MENU_TIMEOUT_SHORT);
            break;
          case alarmdisplay:
            ESP_LOGI(TAG, "Switching from alarmdisplay to sleeptime");
            display_state = sleeptime;
            menu_timer(MENU_TIMOUT);
            break;
          case alarmset:
            ESP_LOGI(TAG, "Toggle alarm on/off");
            alarm_onoff();
            menu_timer(MENU_TIMOUT);
            break;
          case sleeptime:
            ESP_LOGI(TAG, "Switching from sleeptime to brightness display");
            display_state = brightness;
            menu_timer(MENU_TIMOUT);
            break;
          case brightness:
            ESP_LOGI(TAG, "Switching from brightness to Wifi AP display");
            display_state = openwifiap;
            menu_timer(MENU_TIMOUT);
            break;
          case alarmgoing:
            // key is pressed when alarm goes off.
            ESP_LOGI(TAG, "Alarm is going and key is pressed");
            // switch off alarm sound immediatly
            stop_alarm();
            // add sleep immediatly. This disables rearming in in same minute
            alarm_add_sleep();
            // lets look how long we hold the button down
            display_state = alarmgoingpressed;
            menu_timer(ALARM_LONG_PRESS);
            break;
          case openwifiap:
            ESP_LOGI(TAG, "Wifi open ap display keypress");
            // lets look how long we hold the button down
            display_state = openwifiappressed;
            menu_timer(WIFIOPENAP_LONG_PRESS);
            // Long timeout or another press
            break;
          case openwifiappressed:
            ESP_LOGI(TAG, "Switching to clock display from Wifi open AP");
            display_state = clockdisplay;
            break;
          default:
            break;
        }
        break;
      case encoder_press_released:
        ESP_LOGI(TAG, "Key released");
        switch (display_state) {
          case openwifiappressed:
            ESP_LOGI(TAG, "Switching to clock display from Wifi open AP");
            display_state = clockdisplay;
            break;
          default:
            break;
        }
        break;
      case minute_passed:
        ESP_LOGI(TAG, "30 or 60 seconds passed. The time %d:%d:%d", current_timeinfo.tm_hour,
                 current_timeinfo.tm_min, current_timeinfo.tm_sec);
        // Check for alarm or a timed sound and play this.
        if (check_for_alarm()) {
          // we have found an alarm set for this minute and hour. Optional specific for this day
          display_state = alarmgoing;
          // max time alarm is sounding is one hour
          menu_timer(3600000);
        }
        // make sure we check for automatic brightness
        // zero dims during the night
        brightness_set(0);
        // check if we need to store new settings
        clock_store_nvram(-1);
        break;
      // encoder is being turned
      case encoder_up:
        ESP_LOGI(TAG, "Received Encoder up");
        switch (display_state) {
          case alarmset:
            alarm_set_time(1);
            menu_timer(MENU_TIMOUT);
            break;
          case sleeptime:
            sleep_set_time(1);
            menu_timer(MENU_TIMOUT);
            break;
          case brightness:
            brightness_set(1);
            menu_timer(MENU_TIMOUT);
            break;
          default:
            break;
        }
        break;
      // encoder is being turned
      case encoder_down:
        ESP_LOGI(TAG, "Received Encoder down");
        switch (display_state) {
          case alarmset:
            alarm_set_time(-1);
            menu_timer(MENU_TIMOUT);
            break;
          case sleeptime:
            sleep_set_time(-1);
            menu_timer(MENU_TIMOUT);
            break;
          case brightness:
            brightness_set(-1);
            menu_timer(MENU_TIMOUT);
            break;
          default:
            break;
        }
        break;
      case timer_expired:
        ESP_LOGI(TAG, "Received timer expired");
        // The timer we set before expired.
        switch (display_state) {
          case alarmdisplay:
              // We did not press a switch fast enough
              // got to setting the alarm
            display_state = alarmset;
            menu_timer(MENU_TIMOUT);
            break;
          case alarmgoing:
            // should only happen when alarm sounds more than 1 hour
            stop_alarm();
            alarm_off();
            display_state = clockdisplay;
            break;
          case alarmgoingpressed:
            // alarm went off. And we pressed switch. Lets look if long enough
            if (rotary_encoder_get_button() == 0) {
              // we still press the switch. Disable alarm and show it
              alarm_off();
              display_state = alarmgoingoff;
              // show it for two seconds
              menu_timer(2000);
            } else {
              // we only sleep
              display_state = clockdisplay;
            }
            break;
          case openwifiappressed:
            // We are on the WIFI open AP display. Lets look if long enough
            // to open the wifi
            if (rotary_encoder_get_button() == 0) {
              // we still press the switch.
              ESP_LOGI(TAG, "Long keypress. Open AP");
              open_wifi_ap();
            }
            display_state = clockdisplay;
            break;
          default:
            display_state = clockdisplay;
            break;
        }
        break;
      default:
        ESP_LOGI(TAG, "ClockDisplay:Received invalid signal from queue %d",
                 queue_item.disp_task_signal);
        break;
    }

    // now display the various displays. Above we listen to event queue
    // So display updates always happen in reaction to events
    switch (display_state) {
      case clockdisplay:
        ESP_LOGI(TAG, "Clock display");
        time_sendto_display();
        break;
      case alarmdisplay:
        // short display. If we press fast enough go to other settings
        // else go to setting the the alarm
        ESP_LOGI(TAG, "Alarmdisplay");
        max7219_fill_display_buffer("Alarm", MAX7219_ALIGN_MIDDLE);
        max7219_send_display();
        break;
      case alarmset:
        // Only exit from this menu is timeout
        /////// Display alarm time. And if alarm is armed
        ESP_LOGI(TAG, "Alarmset");
        alarm_sendto_display();
        break;
      case sleeptime:
        // display alarm sleep time
        ESP_LOGI(TAG, "Sleeptime");
        sleep_sendto_display();
        break;
      case brightness:
        ESP_LOGI(TAG, "Brightness");
        brightness_sendto_display();
        break;
      case alarmgoing:
        ESP_LOGI(TAG, "Alarm going display");
        time_sendto_display();
        break;
      case alarmgoingpressed:
        ESP_LOGI(TAG, "Alarm pressed display");
        time_sendto_display();
        break;
      case alarmgoingoff:
        // short display to display alarm is off
        ESP_LOGI(TAG, "Alarm switched off display");
        max7219_fill_display_buffer("Off", MAX7219_ALIGN_MIDDLE);
        max7219_send_display();
        break;
      case openwifiap:
        ESP_LOGI(TAG, "Open WIFI AP");
        max7219_fill_display_buffer("Wifi AP", MAX7219_ALIGN_LEFT);
        max7219_send_display();
        break;
      default:
        break;
    }
  }
}

// Just a small routine for easy starting the task from another C file.
void start_display_clock_task() {
  ESP_LOGI(TAG, "Start display clock task");
  xTaskCreate(&display_clock, "DispTask", 4096, NULL, 4, NULL);
}
