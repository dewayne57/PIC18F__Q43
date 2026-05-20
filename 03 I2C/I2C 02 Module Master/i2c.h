/* *****************************************************************************************
 *   File Name: i2c.h
 *   Description: Hardware I2C1 module master implementation for PIC18F47Q43.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-19
 * 
 *   Copyright (c) 2026, Dewayne Hafenstein.
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *   
 *   Pin Configuration:
 *   RC3 - I2C1 SCL (Serial Clock) - routed via PPS
 *   RC4 - I2C1 SDA (Serial Data)  - routed via PPS
 *   RB2 - External IOC interrupt input
 * 
 *   This example uses an MCP23017 I/O expander as the I2C slave device, with Port A connected
 *   to a 8-bit dip switch, and port B connected to 8 LEDs.  Port A is configured as input with 
 *   weak pull-ups enabled and interrupt on change.  If the user changes any of the switches, 
 *   an interrupt is triggered.  The main loop services the request using the interrupt-driven
 *   I2C module and writes the value (inverted) to GPIOB, which updates the LEDs to match the
 *   switch states.
 ***************************************************************************************** */

#ifndef I2C_H
#define I2C_H

#include <stdbool.h>
#include <stdint.h>

/// @brief I2C master handle structure to manage I2C state, configuration, and app-owned buffers.
typedef struct {
    uint16_t speed_khz;         ///< Target I2C bus speed in kHz
    uint8_t retry_count;        ///< Number of retries for I2C operations
    bool initialized;           ///< Flag indicating if I2C interface is initialized
    const uint8_t *tx_buffer;   ///< Application-owned transmit buffer
    uint16_t tx_buffer_size;    ///< Maximum transmit buffer size in bytes
    volatile uint16_t tx_pos;   ///< Current transmit position within tx_buffer
    uint8_t *rx_buffer;         ///< Application-owned receive buffer
    uint16_t rx_buffer_size;    ///< Maximum receive buffer size in bytes
    volatile uint16_t rx_pos;   ///< Current receive position within rx_buffer
} i2c_handle_t;

/// @brief I2C Return codes
typedef enum {
    I2C_SUCCESS = 0,            ///< Operation successful
    I2C_ERROR_TIMEOUT = 1,      ///< Bus timeout (slave holding clock low or module hung)
    I2C_ERROR_NAK = 2,          ///< No acknowledge received from slave
    I2C_ERROR_NOT_INITIALIZED = 3, ///< I2C interface not initialized
    I2C_BUSY = 4,               ///< I2C bus is busy
    I2C_ERROR_ILLEGAL_STATE = 5 ///< Illegal state for the requested operation
} i2c_status_t;

/// @brief Initialize the I2C1 hardware module master interface
/// @param handle Pointer to i2c_handle_t structure
/// @param speed_khz Desired I2C bus speed in kHz (for example 100 or 400)
/// @return i2c_status_t indicating success or error
/// @note RC3 is configured as SCL via PPS, RC4 is configured as SDA via PPS
i2c_status_t I2C_Initialize(i2c_handle_t *handle, uint16_t speed_khz);

/// @brief Write the configured transmit buffer to the selected I2C slave
/// @param handle Pointer to i2c_handle_t structure
/// @param device_address 8-bit I2C device address in write form; the driver toggles the low bit as needed
/// @param length Number of bytes to send from the configured transmit buffer
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Write(i2c_handle_t *handle, uint8_t device_address, uint16_t length);

/// @brief Read into the configured receive buffer from the selected I2C slave
/// @param handle Pointer to i2c_handle_t structure
/// @param device_address 8-bit I2C device address in write form; the driver toggles the low bit as needed
/// @param length Number of bytes to receive into the configured receive buffer
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Read(i2c_handle_t *handle, uint8_t device_address, uint16_t length);

#endif // I2C_H
