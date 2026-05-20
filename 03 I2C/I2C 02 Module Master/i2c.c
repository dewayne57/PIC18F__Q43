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
 *   The PIC18F47Q43 I2C1 module is used in 7-bit host mode (MODE = 0b000).  Transfers are
 *   handled by vectored, prioritized interrupts rather than register polling.  The module
 *   automatically generates START, address, data, and STOP conditions.  The driver sets
 *   I2C1ADB0 (address), I2C1CNT (byte count), and I2C1CON0.S (start), then the I2C1 TX/RX
 *   and event/error ISRs complete the transaction.
 *
 *   Clock and timing notes:
 *   This project selects MFINTOSC as the I2C clock source via I2C1CLK.
 *   The bus is configured for Standard-mode operation (100 kHz).
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "i2c.h"

// Maximum number of wakeups while waiting for an interrupt-driven transfer.
#define I2C_WAIT_WAKE_LIMIT 10000U
#define I2C_TARGET_SPEED_KHZ 100U
#define I2C1CLK_SRC_MFINTOSC 0x03U

/// @brief A definition of the I2C master operations and state for interrupt-driven 
/// transfers.  This is used internally by the driver and is not exposed to the user.
typedef enum {
    I2C_OPERATION_NONE = 0,
    I2C_OPERATION_WRITE,
    I2C_OPERATION_READ
} i2c_operation_t;

/// @brief Internal state structure for managing an I2C transfer in the interrupt-driven 
/// driver implementation.
typedef struct {
    volatile i2c_operation_t operation;
    volatile bool active;
    volatile bool done;
    volatile i2c_status_t status;
    volatile uint16_t length;
    volatile uint16_t index;
    const uint8_t *tx_data;
    uint8_t *rx_data;
} i2c_transfer_state_t;

/// @brief Static variable to hold the current I2C transfer state.  This is used by 
/// the ISRs to manage the ongoing transfer.
static i2c_transfer_state_t s_i2c_transfer = {
    .operation = I2C_OPERATION_NONE,
    .active = false,
    .done = true,
    .status = I2C_SUCCESS,
    .length = 0U,
    .index = 0U,
    .tx_data = NULL,
    .rx_data = NULL
};

/// @brief Disable all I2C interrupt sources
/// @param None
/// @return None
static void i2c_disable_irq_sources(void)
{
    PIE7bits.I2C1RXIE = 0;
    PIE7bits.I2C1TXIE = 0;
    PIE7bits.I2C1IE = 0;
    PIE7bits.I2C1EIE = 0;
}

/// @brief Enable internal I2C1 event sources that feed the module event interrupt.
/// @param None
/// @return None
static void i2c_enable_module_events(void)
{
    I2C1PIE = 0x00U;
    I2C1PIEbits.SCIE = 1U;
    I2C1PIEbits.PCIE = 1U;
    I2C1PIEbits.ADRIE = 1U;
    I2C1PIEbits.WRIE = 1U;
    I2C1PIEbits.ACKTIE = 1U;
    I2C1PIEbits.CNTIE = 1U;
}

/// @brief Finish the current I2C transfer and update the transfer state
/// @param status The status of the completed transfer
/// @return None
static void i2c_finish_transfer(i2c_status_t status)
{
    i2c_disable_irq_sources();
    s_i2c_transfer.status = status;
    s_i2c_transfer.active = false;
    s_i2c_transfer.done = true;
    s_i2c_transfer.operation = I2C_OPERATION_NONE;
    LATDbits.LATD1 = 1U;

    if (status == I2C_SUCCESS)
    {
        LATDbits.LATD3 = 1U;
    }
}

/// @brief Wait for the current I2C transfer to complete or timeout
/// @param None 
/// @return i2c_status_t indicating success or error
static i2c_status_t i2c_wait_transfer_complete(void)
{
    uint16_t wake_count = 0U;

    while (!s_i2c_transfer.done)
    {
        if (wake_count >= I2C_WAIT_WAKE_LIMIT)
        {
            i2c_finish_transfer(I2C_ERROR_TIMEOUT);
            break;
        }

        wake_count++;
        SLEEP();
        NOP();
    }

    return s_i2c_transfer.status;
}

/// @brief I2C1 Transmit Interrupt Service Routine
/// @param None
/// @return None
void __interrupt(irq(IRQ_I2C1TX), high_priority) I2C1_TX_ISR(void)
{
    if ((PIE7bits.I2C1TXIE == 0U) || (PIR7bits.I2C1TXIF == 0U))
    {
        return;
    }

    LATDbits.LATD2 ^= 1U;

    PIR7bits.I2C1TXIF = 0U;

    if ((!s_i2c_transfer.active) || (s_i2c_transfer.operation != I2C_OPERATION_WRITE))
    {
        return;
    }

    if (I2C1ERRbits.NACKIF != 0U)
    {
        I2C1ERRbits.NACKIF = 0U;
        LATDbits.LATD3 = 0U;
        i2c_finish_transfer(I2C_ERROR_NAK);
        return;
    }

    if (s_i2c_transfer.index < s_i2c_transfer.length)
    {
        I2C1TXB = s_i2c_transfer.tx_data[s_i2c_transfer.index];
        s_i2c_transfer.index++;
    }
    else
    {
        PIE7bits.I2C1TXIE = 0U;
    }
}

/// @brief I2C1 Receive Interrupt Service Routine
/// @param None
/// @return None
void __interrupt(irq(IRQ_I2C1RX), high_priority) I2C1_RX_ISR(void)
{
    if ((PIE7bits.I2C1RXIE == 0U) || (PIR7bits.I2C1RXIF == 0U))
    {
        return;
    }

    LATDbits.LATD2 ^= 1U;

    PIR7bits.I2C1RXIF = 0U;

    if ((!s_i2c_transfer.active) || (s_i2c_transfer.operation != I2C_OPERATION_READ))
    {
        return;
    }

    if (s_i2c_transfer.index < s_i2c_transfer.length)
    {
        s_i2c_transfer.rx_data[s_i2c_transfer.index] = I2C1RXB;
        s_i2c_transfer.index++;
    }

    if (s_i2c_transfer.index >= s_i2c_transfer.length)
    {
        PIE7bits.I2C1RXIE = 0U;
    }
}

/// @brief I2C1 Event Interrupt Service Routine
/// @param None
/// @return None
void __interrupt(irq(IRQ_I2C1), high_priority) I2C1_EVENT_ISR(void)
{
    if ((PIE7bits.I2C1IE == 0U) || (PIR7bits.I2C1IF == 0U))
    {
        return;
    }

    LATDbits.LATD2 ^= 1U;

    PIR7bits.I2C1IF = 0U;

    if ((!s_i2c_transfer.active) || (s_i2c_transfer.done))
    {
        return;
    }

    if (I2C1ERRbits.NACKIF != 0U)
    {
        I2C1ERRbits.NACKIF = 0U;
        i2c_finish_transfer(I2C_ERROR_NAK);
        return;
    }

    if (I2C1PIRbits.PCIF != 0U)
    {
        I2C1PIRbits.PCIF = 0U;
        i2c_finish_transfer(I2C_SUCCESS);
    }
}

/// @brief I2C1 Error Interrupt Service Routine
/// @param None
/// @return None
void __interrupt(irq(IRQ_I2C1E), high_priority) I2C1_ERROR_ISR(void)
{
    if ((PIE7bits.I2C1EIE == 0U) || (PIR7bits.I2C1EIF == 0U))
    {
        return;
    }

    LATDbits.LATD2 ^= 1U;

    PIR7bits.I2C1EIF = 0U;

    if (!s_i2c_transfer.active)
    {
        return;
    }

    if (I2C1ERRbits.NACKIF != 0U)
    {
        I2C1ERRbits.NACKIF = 0U;
        LATDbits.LATD3 = 0U;
        i2c_finish_transfer(I2C_ERROR_NAK);
        return;
    }

    i2c_finish_transfer(I2C_ERROR_TIMEOUT);
}

/// @brief Write data to an I2C slave device
/// @param address I2C slave address
/// @param data Pointer to data buffer to write
/// @param length Number of bytes to write
/// @return i2c_status_t indicating success or error
static i2c_status_t i2c_do_write(uint8_t address, const uint8_t *data, uint16_t length) {
    if ((data == NULL) || (length == 0U) || (length > 255U))
    {
        return I2C_ERROR_TIMEOUT;
    }

    if (s_i2c_transfer.active)
    {
        return I2C_ERROR_TIMEOUT;
    }

    s_i2c_transfer.operation = I2C_OPERATION_WRITE;
    s_i2c_transfer.active = true;
    s_i2c_transfer.done = false;
    s_i2c_transfer.status = I2C_SUCCESS;
    s_i2c_transfer.length = length;
    s_i2c_transfer.index = 1U;
    s_i2c_transfer.tx_data = data;
    s_i2c_transfer.rx_data = NULL;

    I2C1PIR = 0x00U;
    I2C1ERR = 0x00U;
    PIR7bits.I2C1TXIF = 0U;
    PIR7bits.I2C1RXIF = 0U;
    PIR7bits.I2C1IF = 0U;
    PIR7bits.I2C1EIF = 0U;
    i2c_enable_module_events();
    i2c_enable_module_events();

    LATDbits.LATD1 = 0U;
    LATDbits.LATD3 = 1U;

    IPR7bits.I2C1TXIP = 1U;
    IPR7bits.I2C1RXIP = 1U;
    IPR7bits.I2C1IP = 1U;
    IPR7bits.I2C1EIP = 1U;

    PIE7bits.I2C1RXIE = 0U;
    PIE7bits.I2C1TXIE = 1U;
    PIE7bits.I2C1IE = 1U;
    PIE7bits.I2C1EIE = 1U;

    LATDbits.LATD1 = 0U;
    LATDbits.LATD3 = 1U;

    I2C1ADB0 = address & 0xFEU;
    I2C1CNT = (uint8_t)length;
    I2C1TXB = data[0];
    I2C1CON0bits.S = 1;

    return i2c_wait_transfer_complete();
}

/// @brief Read data from an I2C slave device
/// @param address I2C slave address
/// @param data Pointer to data buffer to read into
/// @param length Number of bytes to read
/// @return i2c_status_t indicating success or error
static i2c_status_t i2c_do_read(uint8_t address, uint8_t *data, uint16_t length) {
    if ((data == NULL) || (length == 0U) || (length > 255U))
    {
        return I2C_ERROR_TIMEOUT;
    }

    if (s_i2c_transfer.active)
    {
        return I2C_ERROR_TIMEOUT;
    }

    s_i2c_transfer.operation = I2C_OPERATION_READ;
    s_i2c_transfer.active = true;
    s_i2c_transfer.done = false;
    s_i2c_transfer.status = I2C_SUCCESS;
    s_i2c_transfer.length = length;
    s_i2c_transfer.index = 0U;
    s_i2c_transfer.tx_data = NULL;
    s_i2c_transfer.rx_data = data;

    I2C1PIR = 0x00U;
    I2C1ERR = 0x00U;
    PIR7bits.I2C1TXIF = 0U;
    PIR7bits.I2C1RXIF = 0U;
    PIR7bits.I2C1IF = 0U;
    PIR7bits.I2C1EIF = 0U;

    IPR7bits.I2C1TXIP = 1U;
    IPR7bits.I2C1RXIP = 1U;
    IPR7bits.I2C1IP = 1U;
    IPR7bits.I2C1EIP = 1U;

    PIE7bits.I2C1TXIE = 0U;
    PIE7bits.I2C1RXIE = 1U;
    PIE7bits.I2C1IE = 1U;
    PIE7bits.I2C1EIE = 1U;

    LATDbits.LATD1 = 0U;
    LATDbits.LATD3 = 1U;

    I2C1ADB0 = address | 0x01U;
    I2C1CNT = (uint8_t)length;
    I2C1CON0bits.S = 1;

    return i2c_wait_transfer_complete();
}

// --- Public API ---
/// @brief Initialize the I2C1 hardware module master interface
/// @param handle Pointer to i2c_handle_t structure to initialize
/// @param speed_khz Desired I2C bus speed in kHz (for example 100 or 400)
/// @return i2c_status_t indicating success or error    
i2c_status_t I2C_Initialize(i2c_handle_t *handle, uint16_t speed_khz) {
    if ((handle == NULL) || (speed_khz == 0U)) return I2C_ERROR_NOT_INITIALIZED;

    I2C1CON0bits.EN = 0;
    I2C1CLK = I2C1CLK_SRC_MFINTOSC;
    I2C1CON0bits.MODE = 0b000;
    I2C1CON2bits.ABD = 0U;
    I2C1CON2bits.SDAHT = 0b01U;
    I2C1PIR = 0x00U; I2C1ERR = 0x00U;
    i2c_enable_module_events();
    I2C1CON0bits.EN = 1;
    LATDbits.LATD1 = 1U;
    LATDbits.LATD2 = 1U;
    LATDbits.LATD3 = 1U;
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
