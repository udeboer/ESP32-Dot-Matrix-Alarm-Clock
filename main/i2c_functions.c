/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// Simple init and support functions for i2c master functionality
//
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "freertos/semphr.h"
#include "hal/i2c_types.h"

// Own headers
#include "i2c_functions.h"

// Set logging TAG per module
static const char *TAG = "I2C functions";

typedef struct {
  SemaphoreHandle_t i2c_busy_mutex;  // access to the i2c bus with only 1 thread
  i2c_port_t port;
  i2c_config_t port_conf;
} i2c_port_desc_t;

static i2c_port_desc_t i2c_port_desc = {.i2c_busy_mutex = NULL, .port = 0};

// 2 functions below are called to get sole access to the bus
esp_err_t i2c_take_port(void) {
  if (i2c_port_desc.i2c_busy_mutex != NULL) {
    if (xSemaphoreTake(i2c_port_desc.i2c_busy_mutex, 100 / portTICK_PERIOD_MS) == pdTRUE) {
      ESP_LOGI(TAG, "Mutex taken");
      return ESP_OK;
    }
  } else {
    ESP_LOGE(TAG, "I2C access when port is not initialized");
  }
  return ESP_FAIL;
}
esp_err_t i2c_give_port(void) {
  if (xSemaphoreGive(i2c_port_desc.i2c_busy_mutex) == pdTRUE) {
    ESP_LOGI(TAG, "Mutex returned");
    return ESP_OK;
  }
  return ESP_FAIL;
}

esp_err_t i2c_init_port(void) {
  if (i2c_port_desc.i2c_busy_mutex == NULL) {
    i2c_port_desc.i2c_busy_mutex = xSemaphoreCreateMutex();
    if (i2c_port_desc.i2c_busy_mutex == NULL) {
      ESP_LOGE(TAG, "Error: I2C could not be initialized. Could not get mutex");
      return ESP_FAIL;
    }
    i2c_port_desc.port = I2C_PORT;
    i2c_port_desc.port_conf.mode = I2C_MODE_MASTER;
    i2c_port_desc.port_conf.sda_io_num = I2C_SDA_GPIO;
    i2c_port_desc.port_conf.sda_pullup_en = I2C_PULLUP;
    i2c_port_desc.port_conf.scl_io_num = I2C_SCL_GPIO;
    i2c_port_desc.port_conf.scl_pullup_en = I2C_PULLUP;
    i2c_port_desc.port_conf.master.clk_speed = I2C_FREQ_HZ;  // I2C frequency
    i2c_param_config(i2c_port_desc.port, &i2c_port_desc.port_conf);
    i2c_driver_install(i2c_port_desc.port, i2c_port_desc.port_conf.mode, 0, 0, 0);
    return ESP_OK;
  } else {
    ESP_LOGE(TAG, "Error: I2C already initialized.");
    return ESP_FAIL;
  }
}

// data_to_send can be used to first write register to read.
esp_err_t i2c_read(const int dev_addr, const void *data_to_send, size_t size_to_send,
                   void *data_to_recv, size_t size_to_recv) {
  // take mutex
  if (i2c_take_port() == ESP_OK) {
    // start creating packet to send
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    // check if we are only receiving. Most of the time we need to
    // send a command or register value
    if ((data_to_send != NULL) && (size_to_send != 0)) {
      i2c_master_start(cmd);
      // I2C Address is 7bits shifted up. Lower bit is read or write
      i2c_master_write_byte(cmd, dev_addr << 1, true);
      // Write command  or register bytes
      i2c_master_write(cmd, (void *)data_to_send, size_to_send, true);
    }
    i2c_master_start(cmd);
    // Now write to device adress with lower bit set to 1 to tell
    // that next is the read action
    i2c_master_write_byte(cmd, (dev_addr << 1) | 1, true);
    i2c_master_read(cmd, data_to_recv, size_to_recv, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    // The complete exchange of packets is build. Start sending and receiving
    esp_err_t result =
        i2c_master_cmd_begin(i2c_port_desc.port, cmd, I2C_TIMEOUT / portTICK_PERIOD_MS);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "Error in reading from device 0x%02x", dev_addr);
    }

    i2c_cmd_link_delete(cmd);

    i2c_give_port();
    return result;
  } else {
    return ESP_FAIL;
  }
}

// reg_to_send can be NULL. To only send data to the bus.
esp_err_t i2c_write(const int dev_addr, const void *reg_to_send, size_t size_reg_to_send,
                    const void *data_to_send, size_t size_to_send) {
  // take mutex
  if (i2c_take_port() == ESP_OK) {
    // start creating packet to send
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    // I2C Address is 7bits shifted up. Lower bit is read or write
    i2c_master_write_byte(cmd, dev_addr << 1, true);
    if ((reg_to_send != NULL) && (size_reg_to_send != 0)) {
      // need to set register first
      i2c_master_write(cmd, (void *)reg_to_send, size_reg_to_send, true);
    }
    i2c_master_write(cmd, (void *)data_to_send, size_to_send, true);
    i2c_master_stop(cmd);
    // The complete exchange of packets is build. Start sending and receiving
    esp_err_t result =
        i2c_master_cmd_begin(i2c_port_desc.port, cmd, I2C_TIMEOUT / portTICK_PERIOD_MS);
    if (result != ESP_OK) ESP_LOGE(TAG, "Error in reading from device 0x%02x", dev_addr);

    i2c_cmd_link_delete(cmd);

    i2c_give_port();
    return result;
  } else {
    return ESP_FAIL;
  }
}
