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

/// @brief Poll until I2C1 TX buffer is empty (module ready to accept the next byte)
/// @return I2C_SUCCESS, or I2C_ERROR_NAK / I2C_ERROR_TIMEOUT if an error is detected first
static i2c_status_t i2c_wait_txbe(void)
{
    uint16_t timeout = I2C_POLL_TIMEOUT_US;

    while (timeout > 0U)
    {
        // A NACK from the slave causes the module to stop early and set PCIF.
        if (I2C1ERRbits.NACKIF != 0U)
        {
            I2C1ERRbits.NACKIF = 0;
            return I2C_ERROR_NAK;
        }
        if (I2C1STAT1bits.TXBE != 0U)
        {
            return I2C_SUCCESS;
        }
        __delay_us(1);
        timeout--;
    }

    return I2C_ERROR_TIMEOUT;
}

/// @brief Poll until I2C1 RX buffer contains a received byte
/// @return I2C_SUCCESS, or I2C_ERROR_TIMEOUT
static i2c_status_t i2c_wait_rxbf(void)
{
    uint16_t timeout = I2C_POLL_TIMEOUT_US;

    while (timeout > 0U)
    {
        if (I2C1STAT1bits.RXBF != 0U)
        {
            return I2C_SUCCESS;
        }
        __delay_us(1);
        timeout--;
    }

    return I2C_ERROR_TIMEOUT;
}

/// @brief Poll until I2C1 stop condition is complete (PCIF set)
/// @return I2C_SUCCESS, or I2C_ERROR_TIMEOUT
static i2c_status_t i2c_wait_stop(void)
{
    uint16_t timeout = I2C_POLL_TIMEOUT_US;

    while (timeout > 0U)
    {
        if (I2C1PIRbits.PCIF != 0U)
        {
            I2C1PIRbits.PCIF = 0;
            return I2C_SUCCESS;
        }
        __delay_us(1);
        timeout--;
    }

    return I2C_ERROR_TIMEOUT;
}

/// @brief Perform a complete hardware I2C write transaction:
///        START + address(W) + data[0..length-1] + STOP
/// @param address 8-bit I2C address (write bit will be forced clear)
/// @param data Pointer to bytes to send
/// @param length Number of bytes to send
/// @return I2C_SUCCESS, I2C_ERROR_NAK, or I2C_ERROR_TIMEOUT
static i2c_status_t i2c_do_write(uint8_t address, uint8_t *data, uint16_t length)
{
    // Clear all interrupt flags before starting a new transaction.
    I2C1PIR  = 0x00U;
    I2C1ERR  = 0x00U;

    // Load address with write bit (bit 0 = 0).
    I2C1ADB0 = address & 0xFEU;

    // Set the byte count (hardware sends STOP automatically when count reaches 0).
    I2C1CNT = (uint8_t)length;

    // Initiate START condition – the module takes over from here.
    I2C1CON0bits.S = 1;

    // Feed each data byte into the TX buffer.
    for (uint16_t i = 0U; i < length; i++)
    {
        i2c_status_t status = i2c_wait_txbe();
        if (status != I2C_SUCCESS)
        {
            // Wait for the bus to return to idle before returning the error.
            (void)i2c_wait_stop();
            return status;
        }
        I2C1TXB = data[i];
    }

    // Wait for the hardware to issue the STOP condition.
    i2c_status_t status = i2c_wait_stop();
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    // Check for a late NACK (e.g., on the last data byte).
    if (I2C1ERRbits.NACKIF != 0U)
    {
        I2C1ERRbits.NACKIF = 0;
        return I2C_ERROR_NAK;
    }

    return I2C_SUCCESS;
}

/// @brief Perform a complete hardware I2C read transaction:
///        START + address(R) + data[0..length-1] + STOP
/// @param address 8-bit I2C address (read bit will be forced set)
/// @param data Pointer to buffer for received bytes
/// @param length Number of bytes to receive
/// @return I2C_SUCCESS, I2C_ERROR_NAK, or I2C_ERROR_TIMEOUT
static i2c_status_t i2c_do_read(uint8_t address, uint8_t *data, uint16_t length)
{
    // Clear all interrupt flags before starting a new transaction.
    I2C1PIR  = 0x00U;
    I2C1ERR  = 0x00U;

    // Load address with read bit (bit 0 = 1).
    I2C1ADB0 = address | 0x01U;

    // Set the byte count. The module sends NACK + STOP automatically on the last byte.
    I2C1CNT = (uint8_t)length;

    // Initiate START condition.
    I2C1CON0bits.S = 1;

    // Check for NACK after address (slave not present).
    // Give the hardware a brief moment to send the address and receive ACK/NACK.
    __delay_us(50);
    if (I2C1ERRbits.NACKIF != 0U)
    {
        I2C1ERRbits.NACKIF = 0;
        (void)i2c_wait_stop();
        return I2C_ERROR_NAK;
    }

    // Collect each received byte from the RX buffer.
    for (uint16_t i = 0U; i < length; i++)
    {
        i2c_status_t status = i2c_wait_rxbf();
        if (status != I2C_SUCCESS)
        {
            (void)i2c_wait_stop();
            return status;
        }
        data[i] = I2C1RXB;
    }

    // Wait for the hardware to issue the STOP condition.
    return i2c_wait_stop();
}

/// @brief Initialize the I2C1 hardware module master interface
/// @param handle Pointer to i2c_handle_t structure
/// @param speed_khz Desired I2C bus speed in kHz (e.g. 100 or 400)
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Initialize(i2c_handle_t *handle, uint16_t speed_khz)
{
    if ((handle == NULL) || (speed_khz == 0U))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    // Disable the module while configuring it.
    I2C1CON0bits.EN = 0;

    // Select Fosc (64 MHz) as the I2C1 clock source.
    // Clock source 0b0011 = Fosc per the PIC18F47Q43 datasheet.
    I2C1CLK = 0x03U;

    // Calculate and load the baud-rate register.
    // Formula: I2C1BAUD = (Fosc / (4 * BaudRate_Hz)) - 1
    uint32_t baud_val = (_XTAL_FREQ / (4UL * (uint32_t)speed_khz * 1000UL)) - 1UL;
    if (baud_val > 0xFFU)
    {
        baud_val = 0xFFU;
    }
    I2C1BAUD = (uint8_t)baud_val;

    // 7-bit host mode (MODE bits = 0b000).
    I2C1CON0bits.MODE = 0b000;

    // Use I2C1ADB0 as the address buffer (ABD = 0).
    // SDA hold time 300 ns (SDAHT = 0b01) is suitable for standard and fast mode.
    I2C1CON2bits.ABD   = 0U;
    I2C1CON2bits.SDAHT = 0b01U;

    // Clear all flags.
    I2C1PIR = 0x00U;
    I2C1ERR = 0x00U;

    // Enable the I2C1 module.
    I2C1CON0bits.EN = 1;

    // Allow bus lines to settle.
    __delay_us(100);

    handle->speed_khz     = speed_khz;
    handle->half_period_us = (uint16_t)(500U / speed_khz); // Kept for API compatibility
    handle->retry_count   = 3U;
    handle->initialized   = true;

    return I2C_SUCCESS;
}

/// @brief Deinitialize the I2C1 hardware module master interface
/// @param handle Pointer to i2c_handle_t structure
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Deinitialize(i2c_handle_t *handle)
{
    if (handle == NULL)
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    I2C1CON0bits.EN  = 0;
    handle->initialized = false;

    return I2C_SUCCESS;
}

/// @brief START condition stub — the hardware manages START as part of full transactions.
/// @param handle Pointer to i2c_handle_t structure
/// @return I2C_SUCCESS
i2c_status_t I2C_Start(i2c_handle_t *handle)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }
    return I2C_SUCCESS;
}

/// @brief STOP condition stub — the hardware manages STOP as part of full transactions.
/// @param handle Pointer to i2c_handle_t structure
/// @return I2C_SUCCESS
i2c_status_t I2C_Stop(i2c_handle_t *handle)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }
    return I2C_SUCCESS;
}

/// @brief REPEATED START stub — the hardware manages RESTART as part of full transactions.
/// @param handle Pointer to i2c_handle_t structure
/// @return I2C_SUCCESS
i2c_status_t I2C_RestartStart(i2c_handle_t *handle)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }
    return I2C_SUCCESS;
}

/// @brief Send one byte on the I2C bus (single-byte write transaction)
/// @param handle Pointer to i2c_handle_t structure
/// @param address I2C slave address
/// @param data Byte to send
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_SendByte(i2c_handle_t *handle, uint8_t address, uint8_t data)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    return i2c_do_write(address, &data, 1U);
}

/// @brief Send multiple bytes on the I2C bus
/// @param handle Pointer to i2c_handle_t structure
/// @param address I2C slave address
/// @param data Pointer to the data buffer to send
/// @param length Number of bytes to send
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_SendBytes(i2c_handle_t *handle, uint8_t address, uint8_t *data, uint16_t length)
{
    if ((handle == NULL) || (!handle->initialized) || (data == NULL) || (length == 0U))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    return i2c_do_write(address, data, length);
}

/// @brief Probe whether a slave acknowledges its address on the I2C bus
/// @param handle Pointer to i2c_handle_t structure
/// @param address I2C slave address byte with R/W bit clear in bit 0
/// @return I2C_SUCCESS (ACK), I2C_ERROR_NAK, or I2C_ERROR_TIMEOUT
i2c_status_t I2C_ProbeAddress(i2c_handle_t *handle, uint8_t address)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    // Send a zero-payload write transaction — just address + STOP.
    // Set CNT = 0 so the module generates STOP immediately after the address ACK.
    I2C1PIR  = 0x00U;
    I2C1ERR  = 0x00U;
    I2C1ADB0 = address & 0xFEU;
    I2C1CNT  = 0U;
    I2C1CON0bits.S = 1;

    __delay_us(50);

    if (I2C1ERRbits.NACKIF != 0U)
    {
        I2C1ERRbits.NACKIF = 0;
        (void)i2c_wait_stop();
        return I2C_ERROR_NAK;
    }

    return i2c_wait_stop();
}

/// @brief Receive one byte from the I2C bus
/// @param handle Pointer to i2c_handle_t structure
/// @param address I2C slave address
/// @param data Pointer to store received byte
/// @param send_ack Reserved; hardware automatically sends NACK on the last byte
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_ReceiveByte(i2c_handle_t *handle, uint8_t address, uint8_t *data, bool send_ack)
{
    (void)send_ack; // Hardware handles ACK/NACK automatically

    if ((handle == NULL) || (!handle->initialized) || (data == NULL))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    return i2c_do_read(address, data, 1U);
}

/// @brief Receive multiple bytes from the I2C bus
/// @param handle Pointer to i2c_handle_t structure
/// @param address I2C slave address
/// @param data Pointer to the buffer to store received bytes
/// @param length Number of bytes to receive
/// @param send_ack Reserved; hardware automatically sends NACK on the last byte
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_ReceiveBytes(i2c_handle_t *handle, uint8_t address, uint8_t *data, uint16_t length, bool send_ack)
{
    (void)send_ack; // Hardware handles ACK/NACK automatically

    if ((handle == NULL) || (!handle->initialized) || (data == NULL) || (length == 0U))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    return i2c_do_read(address, data, length);
}

/// @brief Send write data then read data using two separate transactions.
/// @param handle Pointer to i2c_handle_t structure
/// @param address I2C slave address (R/W bit set by function)
/// @param write_data Pointer to data to write (typically register address)
/// @param write_length Number of bytes to write
/// @param read_data Pointer to buffer for read data
/// @param read_length Number of bytes to read
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_WriteRead(i2c_handle_t *handle, uint8_t address, uint8_t *write_data, uint16_t write_length, uint8_t *read_data, uint16_t read_length)
{
    if ((handle == NULL) || (!handle->initialized) || (write_data == NULL) || (read_data == NULL))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    i2c_status_t status = i2c_do_write(address, write_data, write_length);
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    return i2c_do_read(address, read_data, read_length);
}

/// @brief Check if SDA is held low by slave (reads the RC4 pin state)
/// @param handle Pointer to i2c_handle_t structure
/// @return true if SDA is low, false if SDA is high
bool I2C_IsSDALow(i2c_handle_t *handle)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return false;
    }

    return (PORTCbits.RC4 == 0);
}

/// @brief Check if SCK is held low by slave (reads the RC3 pin state)
/// @param handle Pointer to i2c_handle_t structure
/// @return true if SCK is low, false if SCK is high
bool I2C_IsSCKLow(i2c_handle_t *handle)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return false;
    }

    return (PORTCbits.RC3 == 0);
}
