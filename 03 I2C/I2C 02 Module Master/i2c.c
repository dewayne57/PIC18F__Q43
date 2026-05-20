/* *****************************************************************************************
 *   File Name: i2c.c
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
 *   RC3 - I2C1 SCL (Serial Clock) - routed via PPS (RC3PPS = 0x37, I2C1SCLPPS = 0x13)
 *   RC4 - I2C1 SDA (Serial Data)  - routed via PPS (RC4PPS = 0x38, I2C1SDAPPS = 0x14)
 * 
 *   I2C1 Module Operation:
 *   The PIC18F47Q43 I2C1 module is used in 7-bit host mode (MODE = 0b000).  All transactions
 *   are polled (no I2C interrupts).  The module automatically generates START, address,
 *   data, and STOP conditions.  The driver sets I2C1ADB0 (address), I2C1CNT (byte count),
 *   and I2C1CON0.S (start), then services the TX or RX buffers until PCIF (stop complete)
 *   is set.
 *
 *   Baud rate formula:
 *   I2C1BAUD = (Fosc / (4 * BaudRate_Hz)) - 1
 *   At 64 MHz, 100 kHz: I2C1BAUD = (64000000 / 400000) - 1 = 159
 *   At 64 MHz, 400 kHz: I2C1BAUD = (64000000 / 1600000) - 1 = 39
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "i2c.h"

// Polling timeout: ~10 ms expressed as a loop count of 1 us iterations
#define I2C_POLL_TIMEOUT_US 10000U

/// @brief Wait for I2C transmit buffer to be empty (ready for next byte)
/// @param  None    
/// @return i2c_status_t indicating success or error
static i2c_status_t i2c_wait_txbe(void) {
    uint16_t timeout = I2C_POLL_TIMEOUT_US;
    while (timeout-- > 0U) {
        if (I2C1ERRbits.NACKIF) { I2C1ERRbits.NACKIF = 0; return I2C_ERROR_NAK; }
        if (I2C1STAT1bits.TXBE) return I2C_SUCCESS;
        __delay_us(1);
    }
    return I2C_ERROR_TIMEOUT;
}

/// @brief Wait for I2C receive buffer to be full (data received)   
/// @param  None
/// @return i2c_status_t indicating success or error
static i2c_status_t i2c_wait_rxbf(void) {
    uint16_t timeout = I2C_POLL_TIMEOUT_US;
    while (timeout-- > 0U) {
        if (I2C1STAT1bits.RXBF) return I2C_SUCCESS;
        __delay_us(1);
    }
    return I2C_ERROR_TIMEOUT;
}

/// @brief Wait for I2C stop condition to complete
/// @param  None
/// @return i2c_status_t indicating success or error
static i2c_status_t i2c_wait_stop(void) {
    uint16_t timeout = I2C_POLL_TIMEOUT_US;
    while (timeout-- > 0U) {
        if (I2C1PIRbits.PCIF) { I2C1PIRbits.PCIF = 0; return I2C_SUCCESS; }
        __delay_us(1);
    }
    return I2C_ERROR_TIMEOUT;
}

/// @brief Write data to an I2C slave device
/// @param address I2C slave address
/// @param data Pointer to data buffer to write
/// @param length Number of bytes to write
/// @return i2c_status_t indicating success or error
static i2c_status_t i2c_do_write(uint8_t address, const uint8_t *data, uint16_t length) {
    I2C1PIR = 0x00U; I2C1ERR = 0x00U;
    I2C1ADB0 = address & 0xFEU;
    I2C1CNT = (uint8_t)length;
    I2C1CON0bits.S = 1;
    for (uint16_t i = 0U; i < length; i++) {
        i2c_status_t status = i2c_wait_txbe();
        if (status != I2C_SUCCESS) { (void)i2c_wait_stop(); return status; }
        I2C1TXB = data[i];
    }
    i2c_status_t status = i2c_wait_stop();
    if (status != I2C_SUCCESS) return status;
    if (I2C1ERRbits.NACKIF) { I2C1ERRbits.NACKIF = 0; return I2C_ERROR_NAK; }
    return I2C_SUCCESS;
}

/// @brief Read data from an I2C slave device
/// @param address I2C slave address
/// @param data Pointer to data buffer to read into
/// @param length Number of bytes to read
/// @return i2c_status_t indicating success or error
static i2c_status_t i2c_do_read(uint8_t address, uint8_t *data, uint16_t length) {
    I2C1PIR = 0x00U; I2C1ERR = 0x00U;
    I2C1ADB0 = address | 0x01U;
    I2C1CNT = (uint8_t)length;
    I2C1CON0bits.S = 1;
    __delay_us(50);
    if (I2C1ERRbits.NACKIF) { I2C1ERRbits.NACKIF = 0; (void)i2c_wait_stop(); return I2C_ERROR_NAK; }
    for (uint16_t i = 0U; i < length; i++) {
        i2c_status_t status = i2c_wait_rxbf();
        if (status != I2C_SUCCESS) { (void)i2c_wait_stop(); return status; }
        data[i] = I2C1RXB;
    }
    return i2c_wait_stop();
}

// --- Public API ---
/// @brief Initialize the I2C1 hardware module master interface
/// @param handle Pointer to i2c_handle_t structure to initialize
/// @param speed_khz Desired I2C bus speed in kHz (for example 100 or 400)
/// @return i2c_status_t indicating success or error    
i2c_status_t I2C_Initialize(i2c_handle_t *handle, uint16_t speed_khz) {
    if ((handle == NULL) || (speed_khz == 0U)) return I2C_ERROR_NOT_INITIALIZED;
    I2C1CON0bits.EN = 0;
    I2C1CLK = 0x03U;
    uint32_t baud_val = (_XTAL_FREQ / (4UL * (uint32_t)speed_khz * 1000UL)) - 1UL;
    if (baud_val > 0xFFU) baud_val = 0xFFU;
    I2C1BAUD = (uint8_t)baud_val;
    I2C1CON0bits.MODE = 0b000;
    I2C1CON2bits.ABD = 0U;
    I2C1CON2bits.SDAHT = 0b01U;
    I2C1PIR = 0x00U; I2C1ERR = 0x00U;
    I2C1CON0bits.EN = 1;
    __delay_us(100);
    handle->speed_khz = speed_khz;
    handle->retry_count = 3U;
    handle->initialized = true;
    return I2C_SUCCESS;
}

/// @brief Write data to an I2C slave device
/// @param handle Pointer to initialized i2c_handle_t structure
/// @param address I2C slave address
/// @param data Pointer to data buffer to write
/// @param length Number of bytes to write
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Write(i2c_handle_t *handle, uint8_t address, const uint8_t *data, uint16_t length) {
    if ((handle == NULL) || (!handle->initialized) || (data == NULL) || (length == 0U)) return I2C_ERROR_NOT_INITIALIZED;
    return i2c_do_write(address, data, length);
}

/// @brief  Read data from an I2C slave device
/// @param handle Pointer to initialized i2c_handle_t structure
/// @param address I2C slave address
/// @param data Pointer to data buffer to read into
/// @param length Number of bytes to read
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Read(i2c_handle_t *handle, uint8_t address, uint8_t *data, uint16_t length) {
    if ((handle == NULL) || (!handle->initialized) || (data == NULL) || (length == 0U)) return I2C_ERROR_NOT_INITIALIZED;
    return i2c_do_read(address, data, length);
}
