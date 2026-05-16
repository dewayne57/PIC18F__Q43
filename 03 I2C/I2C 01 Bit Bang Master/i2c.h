/* *****************************************************************************************
 *   File Name: i2c.h
 *   Description: Bit bang I2C master implementation for PIC18F47Q43.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-11
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
 *   RC3 - I2C SCK (Serial Clock)
 *   RC4 - I2C SDA (Serial Data)
 *   RB2 - External IOC interrupt input
 * 
 *   This example uses an MCP23017 I/O expander as the I2C slave device, with Port A connected
 *   to a 8-bit dip switch, and port B connected to 8 LEDs.  Port A is configured as input with 
 *   weak pull-ups enabled and interrupt on change.  If the user changes any of the switches, 
 *   an interrupt is triggered.  The master will read the state of the MCP23017's GPIOA pins
 *   and write that value (inverted) to GPIOB, which will update the LEDs to match the switch 
 * states.
 ***************************************************************************************** */

#ifndef I2C_H
#define I2C_H

#include <stdbool.h>
#include <stdint.h>

/// @brief I2C master handle structure to manage I2C state and configuration
typedef struct {
    uint16_t speed_khz;         ///< Target I2C bus speed in kHz
    uint16_t half_period_us;    ///< Derived half-period delay in microseconds
    uint8_t retry_count;        ///< Number of retries for I2C operations
    bool initialized;           ///< Flag indicating if I2C interface is initialized
} i2c_handle_t;

/// @brief I2C Return codes
typedef enum {
    I2C_SUCCESS = 0,            ///< Operation successful
    I2C_ERROR_TIMEOUT = 1,      ///< Bus timeout (slave holding clock low)
    I2C_ERROR_NAK = 2,          ///< No acknowledge received from slave
    I2C_ERROR_NOT_INITIALIZED = 3 ///< I2C interface not initialized
} i2c_status_t;

/// @brief Initialize the I2C bit bang master interface
/// @param handle Pointer to i2c_handle_t structure
/// @param speed_khz Desired I2C bus speed in kHz (for example 100 or 1000)
/// @return i2c_status_t indicating success or error
/// @note RC3 is configured as SCK, RC4 is configured as SDA
/// @note Both pins are configured as open-drain outputs (high-impedance when set)
i2c_status_t I2C_Initialize(i2c_handle_t *handle, uint16_t speed_khz);

/// @brief Deinitialize the I2C bit bang master interface
/// @param handle Pointer to i2c_handle_t structure
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Deinitialize(i2c_handle_t *handle);

/// @brief Generate I2C START condition
/// @param handle Pointer to i2c_handle_t structure
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Start(i2c_handle_t *handle);

/// @brief Generate I2C STOP condition
/// @param handle Pointer to i2c_handle_t structure
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Stop(i2c_handle_t *handle);

/// @brief Generate I2C REPEATED START condition
/// @param handle Pointer to i2c_handle_t structure
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_RestartStart(i2c_handle_t *handle);

/// @brief Send one byte on the I2C bus
/// @param handle Pointer to i2c_handle_t structure
/// @param address I2C slave address
/// @param data Byte to send
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_SendByte(i2c_handle_t *handle, uint8_t address, uint8_t data);

/// @brief Send multiple bytes on the I2C bus
/// @param handle Pointer to i2c_handle_t structure
/// @param address I2C slave address
/// @param data Pointer to the data buffer to send
/// @param length Number of bytes to send
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_SendBytes(i2c_handle_t *handle, uint8_t address, uint8_t *data, uint16_t length);

/// @brief Receive one byte from the I2C bus
/// @param handle Pointer to i2c_handle_t structure
/// @param address I2C slave address    
/// @param data Pointer to store received byte
/// @param send_ack If true, send ACK; if false, send NACK
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_ReceiveByte(i2c_handle_t *handle, uint8_t address, uint8_t *data, bool send_ack);

/// @brief Receive multiple bytes from the I2C bus
/// @param handle Pointer to i2c_handle_t structure
/// @param address I2C slave address
/// @param data Pointer to the buffer to store received bytes
/// @param length Number of bytes to receive
/// @param send_ack If true, send ACK after each byte; if false, send NACK after each byte
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_ReceiveBytes(i2c_handle_t *handle, uint8_t address, uint8_t *data, uint16_t length, bool send_ack);

/// @brief Check if SDA is held low by slave
/// @param handle Pointer to i2c_handle_t structure
/// @return true if SDA is low, false if SDA is high
bool I2C_IsSDALow(i2c_handle_t *handle);

/// @brief Check if SCK is held low by slave
/// @param handle Pointer to i2c_handle_t structure
/// @return true if SCK is low, false if SCK is high
bool I2C_IsSCKLow(i2c_handle_t *handle);

#endif // I2C_H
