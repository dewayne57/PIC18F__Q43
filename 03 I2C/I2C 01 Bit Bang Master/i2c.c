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
#include <stdio.h>
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

// Macro to set pin low (driven)
#define I2C_SCK_LOW()   do { I2C_SCK_TRIS &= ~(1 << I2C_SCK_BIT); I2C_SCK_LAT &= ~(1 << I2C_SCK_BIT); } while(0)
#define I2C_SCK_HIGH()  do { I2C_SCK_TRIS |= (1 << I2C_SCK_BIT); } while(0)

#define I2C_SDA_LOW()   do { I2C_SDA_TRIS &= ~(1 << I2C_SDA_BIT); I2C_SDA_LAT &= ~(1 << I2C_SDA_BIT); } while(0)
#define I2C_SDA_HIGH()  do { I2C_SDA_TRIS |= (1 << I2C_SDA_BIT); } while(0)

// Check pin state (1 = high, 0 = low)
#define I2C_SCK_READ()  ((I2C_SCK_PORT >> I2C_SCK_BIT) & 1)
#define I2C_SDA_READ()  ((I2C_SDA_PORT >> I2C_SDA_BIT) & 1)

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

/// @brief Initialize the I2C bit bang master interface
/// @param handle Pointer to i2c_handle_t structure
/// @param clock_delay_us Clock delay in microseconds (affects I2C bus speed)
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Initialize(i2c_handle_t *handle, uint16_t clock_delay_us)
{
    if (handle == NULL)
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    // Configure RC3 and RC4 as digital pins
    ANSELCbits.ANSELC3 = 0;
    ANSELCbits.ANSELC4 = 0;

    // Configure RC3 (SCK) and RC4 (SDA) as open-drain outputs (initially released)
    I2C_SCK_HIGH();  // Release SCK to high state
    I2C_SDA_HIGH();  // Release SDA to high state

    // Store configuration
    handle->clock_delay_us = clock_delay_us;
    handle->retry_count = 3;
    handle->initialized = true;

    // Wait for bus to settle
    i2c_delay(100);

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
    I2C_SDA_HIGH();
    i2c_delay(handle->clock_delay_us);
    I2C_SCK_HIGH();
    i2c_delay(handle->clock_delay_us);

    // START condition: SDA goes low while SCK is high
    I2C_SDA_LOW();
    i2c_delay(handle->clock_delay_us);

    // Then pull SCK low
    I2C_SCK_LOW();
    i2c_delay(handle->clock_delay_us);

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
    i2c_delay(handle->clock_delay_us);

    // Release SCK first
    I2C_SCK_HIGH();
    i2c_delay(handle->clock_delay_us);

    // Then release SDA (STOP condition)
    I2C_SDA_HIGH();
    i2c_delay(handle->clock_delay_us);

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
    i2c_delay(handle->clock_delay_us);
    I2C_SCK_HIGH();
    i2c_delay(handle->clock_delay_us);

    // START condition: SDA goes low while SCK is high
    I2C_SDA_LOW();
    i2c_delay(handle->clock_delay_us);

    // Then pull SCK low
    I2C_SCK_LOW();
    i2c_delay(handle->clock_delay_us);

    return I2C_SUCCESS;
}

/// @brief Send one byte on the I2C bus
/// @param handle Pointer to i2c_handle_t structure
/// @param data Byte to send
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_SendByte(i2c_handle_t *handle, uint8_t data)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    // Send 8 bits, MSB first
    for (uint8_t bit = 0; bit < 8; bit++)
    {
        // Determine bit value (MSB first)
        uint8_t bit_value = (data >> (7 - bit)) & 1;

        // Set SDA based on bit value
        if (bit_value)
        {
            I2C_SDA_HIGH();
        }
        else
        {
            I2C_SDA_LOW();
        }

        i2c_delay(handle->clock_delay_us);

        // Clock pulse: SCK goes high then low
        I2C_SCK_HIGH();
        i2c_delay(handle->clock_delay_us);

        // Check for clock stretching
        i2c_status_t status = i2c_wait_sck_high(handle, 100);
        if (status != I2C_SUCCESS)
        {
            return status;
        }

        i2c_delay(handle->clock_delay_us);
        I2C_SCK_LOW();
        i2c_delay(handle->clock_delay_us);
    }

    // Release SDA and wait for slave ACK
    I2C_SDA_HIGH();
    i2c_delay(handle->clock_delay_us);

    // Clock pulse for ACK bit
    I2C_SCK_HIGH();
    i2c_delay(handle->clock_delay_us);

    // Check for clock stretching
    i2c_status_t status = i2c_wait_sck_high(handle, 100);
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    // Read ACK bit (SDA should be low)
    uint8_t ack_bit = I2C_SDA_READ();

    i2c_delay(handle->clock_delay_us);
    I2C_SCK_LOW();
    i2c_delay(handle->clock_delay_us);

    // Return NAK if slave did not pull SDA low
    if (ack_bit != 0)
    {
        return I2C_ERROR_NAK;
    }

    return I2C_SUCCESS;
}

/// @brief Receive one byte from the I2C bus
/// @param handle Pointer to i2c_handle_t structure
/// @param data Pointer to store received byte
/// @param send_ack If true, send ACK; if false, send NACK
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_ReceiveByte(i2c_handle_t *handle, uint8_t *data, bool send_ack)
{
    if ((handle == NULL) || (!handle->initialized) || (data == NULL))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    uint8_t received_byte = 0;

    // Release SDA and SCK for master to receive
    I2C_SDA_HIGH();
    i2c_delay(handle->clock_delay_us);

    // Receive 8 bits, MSB first
    for (uint8_t bit = 0; bit < 8; bit++)
    {
        // Clock pulse: SCK goes high
        I2C_SCK_HIGH();
        i2c_delay(handle->clock_delay_us);

        // Check for clock stretching
        i2c_status_t status = i2c_wait_sck_high(handle, 100);
        if (status != I2C_SUCCESS)
        {
            return status;
        }

        // Read bit value
        uint8_t bit_value = I2C_SDA_READ();
        received_byte = (received_byte << 1) | bit_value;

        i2c_delay(handle->clock_delay_us);
        I2C_SCK_LOW();
        i2c_delay(handle->clock_delay_us);
    }

    // Send ACK or NACK
    if (send_ack)
    {
        I2C_SDA_LOW();  // Send ACK (pull SDA low)
    }
    else
    {
        I2C_SDA_HIGH(); // Send NACK (release SDA high)
    }

    i2c_delay(handle->clock_delay_us);

    // Clock pulse for ACK/NACK bit
    I2C_SCK_HIGH();
    i2c_delay(handle->clock_delay_us);

    // Check for clock stretching
    i2c_status_t status = i2c_wait_sck_high(handle, 100);
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    i2c_delay(handle->clock_delay_us);
    I2C_SCK_LOW();
    i2c_delay(handle->clock_delay_us);

    // Release SDA
    I2C_SDA_HIGH();

    *data = received_byte;
    return I2C_SUCCESS;
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
