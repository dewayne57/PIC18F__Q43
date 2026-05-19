/* *****************************************************************************************
 *   File Name: i2c.c
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
 *   
 *   Open-Drain Logic:
 *   To drive low:  TRIS = 0, LAT = 0
 *   To release (high): TRIS = 1 (pulled high by external pull-up)
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "i2c.h"

// Port C pin definitions for I2C
#define I2C_SCK_PORT    PORTC
#define I2C_SCK_LAT     LATC
#define I2C_SCK_TRIS    TRISC
#define I2C_SCK_BIT     3

#define I2C_SDA_PORT    PORTC
#define I2C_SDA_LAT     LATC
#define I2C_SDA_TRIS    TRISC
#define I2C_SDA_BIT     4

// Open-drain bit-bang control:
// LOW  -> output low (TRIS=0, LAT=0)
// HIGH -> release line (TRIS=1, external pull-up drives high)
#define I2C_SCK_LOW()   do { I2C_SCK_LAT &= ~(1 << I2C_SCK_BIT); I2C_SCK_TRIS &= ~(1 << I2C_SCK_BIT); } while(0)
#define I2C_SCK_HIGH()  do { I2C_SCK_TRIS |=  (1 << I2C_SCK_BIT); } while(0)

#define I2C_SDA_LOW()   do { I2C_SDA_LAT &= ~(1 << I2C_SDA_BIT); I2C_SDA_TRIS &= ~(1 << I2C_SDA_BIT); } while(0)
#define I2C_SDA_HIGH()  do { I2C_SDA_TRIS |=  (1 << I2C_SDA_BIT); } while(0)

// Check pin state (1 = high, 0 = low)
#define I2C_SCK_READ()  ((I2C_SCK_PORT >> I2C_SCK_BIT) & 1)
#define I2C_SDA_READ()  ((I2C_SDA_PORT >> I2C_SDA_BIT) & 1)

static void i2c_diag_mark(uint8_t code)
{
    (void)code;
}

/// @brief Delay helper function
/// @param delay_us Delay time in microseconds
static void i2c_delay(uint16_t delay_us)
{
    for (uint16_t i = 0; i < delay_us; i++)
    {
        __delay_us(1);
    }
}

/// @brief Wait for SCK to go high (clock stretching)
/// @param handle Pointer to i2c_handle_t structure
/// @param max_wait_ms Maximum time to wait in milliseconds
/// @return i2c_status_t indicating success or timeout
static i2c_status_t i2c_wait_sck_high(i2c_handle_t *handle, uint16_t max_wait_ms)
{
    uint16_t wait_count = 0;
    uint16_t max_waits = max_wait_ms * 10; // Assuming ~100us per iteration

    while ((I2C_SCK_READ() == 0) && (wait_count < max_waits))
    {
        __delay_ms(1);
        wait_count++;
    }

    if (I2C_SCK_READ() == 0)
    {
        i2c_diag_mark(0x0CU);
        return I2C_ERROR_TIMEOUT;
    }

    return I2C_SUCCESS;
}

/// @brief Wait for SDA to go high
/// @param handle Pointer to i2c_handle_t structure
/// @param max_wait_ms Maximum time to wait in milliseconds
/// @return i2c_status_t indicating success or timeout
static i2c_status_t i2c_wait_sda_high(i2c_handle_t *handle, uint16_t max_wait_ms)
{
    uint16_t wait_count = 0;
    uint16_t max_waits = max_wait_ms * 10;

    while ((I2C_SDA_READ() == 0) && (wait_count < max_waits))
    {
        __delay_ms(1);
        wait_count++;
    }

    if (I2C_SDA_READ() == 0)
    {
        return I2C_ERROR_TIMEOUT;
    }

    return I2C_SUCCESS;
}

/// @brief Send one raw byte on the I2C bus and read ACK/NAK
/// @param handle Pointer to i2c_handle_t structure
/// @param data Byte to send
/// @return i2c_status_t indicating success or error
static i2c_status_t i2c_send_raw_byte(i2c_handle_t *handle, uint8_t data)
{
    i2c_diag_mark(0x03U);

    // Send 8 bits, MSB first
    for (uint8_t bit = 0; bit < 8; bit++)
    {
        uint8_t bit_value = (data >> (7 - bit)) & 1U;

        if (bit_value != 0U)
        {
            I2C_SDA_HIGH();
        }
        else
        {
            I2C_SDA_LOW();
        }

        i2c_delay(handle->half_period_us);

        I2C_SCK_HIGH();
        i2c_delay(handle->half_period_us);

        i2c_status_t status = i2c_wait_sck_high(handle, 100);
        if (status != I2C_SUCCESS)
        {
            return status;
        }

        i2c_delay(handle->half_period_us);
        I2C_SCK_LOW();
        i2c_delay(handle->half_period_us);
    }

    // ACK cycle from slave
    I2C_SDA_HIGH();
    i2c_delay(handle->half_period_us);

    I2C_SCK_HIGH();
    i2c_delay(handle->half_period_us);

    i2c_status_t status = i2c_wait_sck_high(handle, 100);
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    uint8_t ack_bit = I2C_SDA_READ();

    i2c_delay(handle->half_period_us);
    I2C_SCK_LOW();
    i2c_delay(handle->half_period_us);

    if (ack_bit != 0U)
    {
        i2c_diag_mark(0x0DU);
        return I2C_ERROR_NAK;
    }

    i2c_diag_mark(0x04U);

    return I2C_SUCCESS;
}

/// @brief Receive one raw byte from the I2C bus
/// @param handle Pointer to i2c_handle_t structure
/// @param data Pointer to store received byte
/// @param send_ack If true, send ACK; if false, send NACK
/// @return i2c_status_t indicating success or error
static i2c_status_t i2c_receive_raw_byte(i2c_handle_t *handle, uint8_t *data, bool send_ack)
{
    uint8_t received_byte = 0U;

    I2C_SDA_HIGH();
    i2c_delay(handle->half_period_us);

    for (uint8_t bit = 0; bit < 8; bit++)
    {
        I2C_SCK_HIGH();
        i2c_delay(handle->half_period_us);

        i2c_status_t status = i2c_wait_sck_high(handle, 100);
        if (status != I2C_SUCCESS)
        {
            return status;
        }

        received_byte <<= 1;
        received_byte |= I2C_SDA_READ();

        i2c_delay(handle->half_period_us);
        I2C_SCK_LOW();
        i2c_delay(handle->half_period_us);
    }

    if (send_ack)
    {
        I2C_SDA_LOW();
    }
    else
    {
        I2C_SDA_HIGH();
    }

    i2c_delay(handle->half_period_us);
    I2C_SCK_HIGH();
    i2c_delay(handle->half_period_us);

    i2c_status_t status = i2c_wait_sck_high(handle, 100);
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    i2c_delay(handle->half_period_us);
    I2C_SCK_LOW();
    i2c_delay(handle->half_period_us);

    I2C_SDA_HIGH();

    *data = received_byte;

    return I2C_SUCCESS;
}

/// @brief Initialize the I2C bit bang master interface
/// @param handle Pointer to i2c_handle_t structure
/// @param speed_khz Desired I2C bus speed in kHz
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Initialize(i2c_handle_t *handle, uint16_t speed_khz)
{
    if ((handle == NULL) || (speed_khz == 0U))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    // Configure RC3 and RC4 as digital open-drain outputs
    ANSELCbits.ANSELC3 = 0;
    ANSELCbits.ANSELC4 = 0;
    ODCONCbits.ODCC3 = 1;
    ODCONCbits.ODCC4 = 1;
    I2C_SCK_LOW();
    I2C_SDA_HIGH();

    // Release both lines high before any transaction.
    I2C_SCK_HIGH();
    I2C_SDA_HIGH();

    // Derive half-period from requested bus speed: T(us) = 1000 / f(kHz)
    // Clamp to at least 1us so high requested speeds still produce a delay.
    uint16_t half_period_us = (uint16_t)(500U / speed_khz);
    if (half_period_us == 0U)
    {
        half_period_us = 1U;
    }

    // Store configuration
    handle->speed_khz = speed_khz;
    handle->half_period_us = half_period_us;
    handle->retry_count = 3;
    handle->initialized = true;

    // Wait for bus to settle
    i2c_delay(100);
    i2c_diag_mark(0x01U);

    return I2C_SUCCESS;
}

/// @brief Deinitialize the I2C bit bang master interface
/// @param handle Pointer to i2c_handle_t structure
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Deinitialize(i2c_handle_t *handle)
{
    if (handle == NULL)
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    // Release both lines to high state
    I2C_SCK_HIGH();
    I2C_SDA_HIGH();

    handle->initialized = false;

    return I2C_SUCCESS;
}

/// @brief Generate I2C START condition
/// @param handle Pointer to i2c_handle_t structure
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Start(i2c_handle_t *handle)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    // Ensure both lines are high initially
    i2c_diag_mark(0x02U);
    I2C_SDA_HIGH();
    i2c_delay(handle->half_period_us);
    I2C_SCK_HIGH();
    i2c_delay(handle->half_period_us);

    // START condition: SDA goes low while SCK is high
    I2C_SDA_LOW();
    i2c_delay(handle->half_period_us);

    // Then pull SCK low
    I2C_SCK_LOW();
    i2c_delay(handle->half_period_us);

    i2c_diag_mark(0x05U);

    return I2C_SUCCESS;
}

/// @brief Generate I2C STOP condition
/// @param handle Pointer to i2c_handle_t structure
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Stop(i2c_handle_t *handle)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    // STOP condition: SDA goes high while SCK is high
    // First ensure SDA is low and SCK is low
    I2C_SDA_LOW();
    I2C_SCK_LOW();
    i2c_delay(handle->half_period_us);

    // Release SCK first
    I2C_SCK_HIGH();
    i2c_delay(handle->half_period_us);

    // Then release SDA (STOP condition)
    I2C_SDA_HIGH();
    i2c_delay(handle->half_period_us);

    return I2C_SUCCESS;
}

/// @brief Generate I2C REPEATED START condition
/// @param handle Pointer to i2c_handle_t structure
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_RestartStart(i2c_handle_t *handle)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    // Repeated START: SDA goes low while SCK is high
    I2C_SDA_HIGH();
    i2c_delay(handle->half_period_us);
    I2C_SCK_HIGH();
    i2c_delay(handle->half_period_us);

    // START condition: SDA goes low while SCK is high
    I2C_SDA_LOW();
    i2c_delay(handle->half_period_us);

    // Then pull SCK low
    I2C_SCK_LOW();
    i2c_delay(handle->half_period_us);

    return I2C_SUCCESS;
}

/// @brief Send one byte on the I2C bus
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

    i2c_status_t status = I2C_Start(handle);
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    i2c_diag_mark(0x06U);
    status = i2c_send_raw_byte(handle, address & 0xFEU);
    if (status != I2C_SUCCESS)
    {
        if (status == I2C_ERROR_NAK)
        {
            i2c_diag_mark(0x08U);
        }
        (void)I2C_Stop(handle);
        return status;
    }

    i2c_diag_mark(0x07U);
    status = i2c_send_raw_byte(handle, data);
    if (status != I2C_SUCCESS)
    {
        if (status == I2C_ERROR_NAK)
        {
            i2c_diag_mark(0x09U);
        }
        (void)I2C_Stop(handle);
        return status;
    }

    return I2C_Stop(handle);
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

    i2c_status_t status = I2C_Start(handle);
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    i2c_diag_mark(0x06U);
    status = i2c_send_raw_byte(handle, address & 0xFEU);
    if (status != I2C_SUCCESS)
    {
        if (status == I2C_ERROR_NAK)
        {
            i2c_diag_mark(0x08U);
        }
        (void)I2C_Stop(handle);
        return status;
    }

    for (uint16_t i = 0; i < length; i++)
    {
        i2c_diag_mark(0x07U);
        status = i2c_send_raw_byte(handle, data[i]);
        if (status != I2C_SUCCESS)
        {
            if (status == I2C_ERROR_NAK)
            {
                i2c_diag_mark(0x09U);
            }
            (void)I2C_Stop(handle);
            return status;
        }
    }

    return I2C_Stop(handle);
}

i2c_status_t I2C_ProbeAddress(i2c_handle_t *handle, uint8_t address)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    i2c_status_t status = I2C_Start(handle);
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    i2c_diag_mark(0x06U);
    status = i2c_send_raw_byte(handle, address & 0xFEU);
    if (status != I2C_SUCCESS)
    {
        if (status == I2C_ERROR_NAK)
        {
            i2c_diag_mark(0x08U);
        }

        (void)I2C_Stop(handle);
        return status;
    }

    i2c_diag_mark(0x04U);
    return I2C_Stop(handle);
}

/// @brief Receive one byte from the I2C bus
/// @param handle Pointer to i2c_handle_t structure
/// @param address I2C slave address
/// @param data Pointer to store received byte
/// @param send_ack If true, send ACK; if false, send NACK
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_ReceiveByte(i2c_handle_t *handle, uint8_t address, uint8_t *data, bool send_ack)
{
    if ((handle == NULL) || (!handle->initialized) || (data == NULL))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    i2c_status_t status = I2C_Start(handle);
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    i2c_diag_mark(0x06U);
    status = i2c_send_raw_byte(handle, address | 0x01U);
    if (status != I2C_SUCCESS)
    {
        if (status == I2C_ERROR_NAK)
        {
            i2c_diag_mark(0x08U);
        }
        (void)I2C_Stop(handle);
        return status;
    }

    status = i2c_receive_raw_byte(handle, data, send_ack);
    if (status != I2C_SUCCESS)
    {
        (void)I2C_Stop(handle);
        return status;
    }

    return I2C_Stop(handle);
}

/// @brief Receive multiple bytes from the I2C bus
/// @param handle Pointer to i2c_handle_t structure
/// @param address I2C slave address
/// @param data Pointer to the buffer to store received bytes
/// @param length Number of bytes to receive
/// @param send_ack If true, send ACK after each byte; if false, send NACK after each byte
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_ReceiveBytes(i2c_handle_t *handle, uint8_t address, uint8_t *data, uint16_t length, bool send_ack)
{
    if ((handle == NULL) || (!handle->initialized) || (data == NULL) || (length == 0U))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    i2c_status_t status = I2C_Start(handle);
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    i2c_diag_mark(0x06U);
    status = i2c_send_raw_byte(handle, address | 0x01U);
    if (status != I2C_SUCCESS)
    {
        if (status == I2C_ERROR_NAK)
        {
            i2c_diag_mark(0x08U);
        }
        (void)I2C_Stop(handle);
        return status;
    }

    for (uint16_t i = 0; i < length; i++)
    {
        bool ack = send_ack && (i < (length - 1U));
        status = i2c_receive_raw_byte(handle, &data[i], ack);
        if (status != I2C_SUCCESS)
        {
            (void)I2C_Stop(handle);
            return status;
        }
    }

    return I2C_Stop(handle);
}

i2c_status_t I2C_WriteRead(i2c_handle_t *handle, uint8_t address, uint8_t *write_data, uint16_t write_length, uint8_t *read_data, uint16_t read_length)
{
    if ((handle == NULL) || (!handle->initialized) || (write_data == NULL) || (read_data == NULL))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    i2c_status_t status = I2C_Start(handle);
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    // Send write address byte
    i2c_diag_mark(0x06U);
    status = i2c_send_raw_byte(handle, address & 0xFEU);
    if (status != I2C_SUCCESS)
    {
        if (status == I2C_ERROR_NAK)
        {
            i2c_diag_mark(0x08U);
        }
        (void)I2C_Stop(handle);
        return status;
    }

    // Send write data
    for (uint16_t i = 0; i < write_length; i++)
    {
        status = i2c_send_raw_byte(handle, write_data[i]);
        if (status != I2C_SUCCESS)
        {
            if (status == I2C_ERROR_NAK)
            {
                i2c_diag_mark(0x09U);
            }
            (void)I2C_Stop(handle);
            return status;
        }
    }

    // REPEATED START (SDA goes high while SCL is high, then SCL goes low)
    status = I2C_RestartStart(handle);
    if (status != I2C_SUCCESS)
    {
        (void)I2C_Stop(handle);
        return status;
    }

    // Send read address byte (with R/W bit set)
    status = i2c_send_raw_byte(handle, address | 0x01U);
    if (status != I2C_SUCCESS)
    {
        if (status == I2C_ERROR_NAK)
        {
            i2c_diag_mark(0x08U);
        }
        (void)I2C_Stop(handle);
        return status;
    }

    // Receive read data
    for (uint16_t i = 0; i < read_length; i++)
    {
        bool send_ack = (i < (read_length - 1U));
        status = i2c_receive_raw_byte(handle, &read_data[i], send_ack);
        if (status != I2C_SUCCESS)
        {
            (void)I2C_Stop(handle);
            return status;
        }
    }

    return I2C_Stop(handle);
}

/// @brief Check if SDA is held low by slave
/// @param handle Pointer to i2c_handle_t structure
/// @return true if SDA is low, false if SDA is high
bool I2C_IsSDALow(i2c_handle_t *handle)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return false;
    }

    return (I2C_SDA_READ() == 0);
}

/// @brief  Check if SCK is held low by slave   
/// @param handle Pointer to i2c_handle_t structure
/// @return true if SCK is low, false if SCK is high    
bool I2C_IsSCKLow(i2c_handle_t *handle)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return false;
    }

    return (I2C_SCK_READ() == 0);
}
