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
 *   an interrupt is triggered.  The master will read the state of the MCP23017's GPIOA pins
 *   and write that value (inverted) to GPIOB, which will update the LEDs to match the switch 
 *   states.
 ***************************************************************************************** */

#ifndef I2C_H
#define I2C_H

#include <stdbool.h>
#include <stdint.h>

/// @brief I2C master handle structure to manage I2C state and configuration
typedef struct {
    uint16_t speed_khz;         ///< Target I2C bus speed in kHz
    uint8_t retry_count;        ///< Number of retries for I2C operations
    bool initialized;           ///< Flag indicating if I2C interface is initialized
} i2c_handle_t;

/// @brief I2C Return codes
typedef enum {
    I2C_SUCCESS = 0,            ///< Operation successful
    I2C_ERROR_TIMEOUT = 1,      ///< Bus timeout (slave holding clock low or module hung)
    I2C_ERROR_NAK = 2,          ///< No acknowledge received from slave
    I2C_ERROR_NOT_INITIALIZED = 3 ///< I2C interface not initialized
} i2c_status_t;

/// @brief Initialize the I2C1 hardware module master interface
/// @param handle Pointer to i2c_handle_t structure
/// @param speed_khz Desired I2C bus speed in kHz (for example 100 or 400)
/// @return i2c_status_t indicating success or error
/// @note RC3 is configured as SCL via PPS, RC4 is configured as SDA via PPS
i2c_status_t I2C_Initialize(i2c_handle_t *handle, uint16_t speed_khz);

/// @brief Write data to an I2C slave (handles start/stop automatically)
/// @param handle Pointer to i2c_handle_t structure
/// @param address I2C slave address (7-bit, left-aligned)
/// @param data Pointer to the data buffer to send
/// @param length Number of bytes to send
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Write(i2c_handle_t *handle, uint8_t address, const uint8_t *data, uint16_t length);

/// @brief Read data from an I2C slave (handles start/stop automatically)
/// @param handle Pointer to i2c_handle_t structure
/// @param address I2C slave address (7-bit, left-aligned)
/// @param data Pointer to the buffer to store received bytes
/// @param length Number of bytes to receive
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Read(i2c_handle_t *handle, uint8_t address, uint8_t *data, uint16_t length);

#endif // I2C_H
