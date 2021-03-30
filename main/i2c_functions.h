/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef I2C_FUNC_H_
#define I2C_FUNC_H_
// Simple init and support functions for i2c master functionality
// Only 1 bus is defined
// Devices use this to init and share the i2c port

#include "driver/i2c.h"
#include "esp_err.h"
#include "hal/gpio_types.h"

#define I2C_PORT I2C_NUM_1  // I2C port _0 or _1 
#define I2C_SCL_GPIO 19
#define I2C_SDA_GPIO 23
#define I2C_FREQ_HZ 100000  // master clock frequency
// ESP32 can provide pullup resistor on bus. GPIO_PULLUP_ENABLE
#define I2C_PULLUP GPIO_PULLUP_DISABLE
// Timeout for all I2C operations in milliseconds
#define I2C_TIMEOUT 100


/* Init I2C port and init Mutex. Values used to initalize port are
 * taken from this header file. Mutex is used to make sure that only one thread is 
 * using the I2C port.
 */
esp_err_t i2c_init_port(void);

/* Read data from I2C device. 
 * dev_addr is device address 7 bits (not shifted up). 
 * data_to_send and its size can be used to send register byte first.
 * When not using data_to_send it should be set to NULL and size 0.
 * data_to recv size should be known and allocated before.
 */
esp_err_t i2c_read(const int dev_addr, const void *data_to_send, size_t size_to_send,
                   void *data_to_recv, size_t size_to_recv);

/* Write data to I2C device. 
 * dev_addr is device address 7 bits (not shifted up). 
 * reg_to_send and its size can be used to send register byte first.
 * When not using reg_to_send it should be set to NULL and size 0.
 */
esp_err_t i2c_write(const int dev_addr, const void *reg_to_send, size_t size_reg_to_send,
                    const void *data_to_send, size_t size_to_send);

#endif
