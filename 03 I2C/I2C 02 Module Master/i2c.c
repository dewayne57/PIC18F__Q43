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
 *   The PIC18F47Q43 I2C1 module is used in 7-bit host mode (MODE = 0b100).  Transfers are
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
#include <stdio.h>
#include "config.h"
#include "../../Libraries/PPSLIB/pps.h"
#include "i2c.h"

// Maximum number of wakeups while waiting for an interrupt-driven transfer.
#define I2C_WAIT_WAKE_LIMIT 10000U
#define I2C_TARGET_SPEED_KHZ 100U
#define I2C1CLK_SRC_MFINTOSC 0x03U
#define I2C_MODE_MASTER_7BIT 0b100

/// @brief A definition of the I2C master operations and state for interrupt-driven
/// transfers.  This is used internally by the driver and is not exposed to the user.
typedef enum
{
    I2C_OPERATION_NONE = 0,
    I2C_OPERATION_WRITE,
    I2C_OPERATION_READ
} i2c_operation_t;

/// @brief Internal state structure for managing an I2C transfer in the interrupt-driven
/// driver implementation.
typedef struct
{
    volatile i2c_operation_t operation;
    volatile bool active;
    volatile bool done;
    volatile i2c_status_t status;
    volatile uint16_t length;
} i2c_transfer_state_t;

/// @brief Static variable to hold the current I2C transfer state.  This is used by
/// the ISRs to manage the ongoing transfer.
static i2c_transfer_state_t s_i2c_transfer = {
    .operation = I2C_OPERATION_NONE,
    .active = false,
    .done = true,
    .status = I2C_SUCCESS,
    .length = 0U};

/// @brief Pointer to the application-owned handle for the active transfer.
static i2c_handle_t *s_active_handle = NULL;

/// @brief Disable all I2C interrupt sources.
static void i2c_disable_irq_sources(void)
{
    PIE7bits.I2C1RXIE = 0U;
    PIE7bits.I2C1TXIE = 0U;
    PIE7bits.I2C1IE = 0U;
    PIE7bits.I2C1EIE = 0U;
}

/// @brief Enable internal I2C1 event sources that feed the module event interrupt.
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

/// @brief Finish the current I2C transfer and update the transfer state.
static void i2c_finish_transfer(i2c_status_t status)
{
    i2c_disable_irq_sources();
    s_i2c_transfer.status = status;
    s_i2c_transfer.active = false;
    s_i2c_transfer.done = true;
    s_i2c_transfer.operation = I2C_OPERATION_NONE;
    s_i2c_transfer.length = 0U;
    s_active_handle = NULL;
    LATDbits.LATD0 = 1U;
    LATDbits.LATD1 = 1U;

    if (status == I2C_SUCCESS)
    {
        LATDbits.LATD3 = 1U;
    }
}

/// @brief Wait for the current I2C transfer to complete or timeout.
static i2c_status_t i2c_wait_transfer_complete(void)
{
    uint16_t wake_count = 0U;

    while (!s_i2c_transfer.done)
    {
        if (wake_count >= I2C_WAIT_WAKE_LIMIT)
        {
             printf("I2C timeout: op=%u TXIE=%u RXIE=%u I2CIE=%u EIE=%u TXIF=%u RXIF=%u I2CIF=%u EIF=%u PCIF=%u NACK=%u CON0=%02X CON1=%02X CON2=%02X CLK=%02X\r\n",
                   (unsigned)s_i2c_transfer.operation,
                   (unsigned)PIE7bits.I2C1TXIE,
                   (unsigned)PIE7bits.I2C1RXIE,
                   (unsigned)PIE7bits.I2C1IE,
                   (unsigned)PIE7bits.I2C1EIE,
                   (unsigned)PIR7bits.I2C1TXIF,
                   (unsigned)PIR7bits.I2C1RXIF,
                   (unsigned)PIR7bits.I2C1IF,
                   (unsigned)PIR7bits.I2C1EIF,
                   (unsigned)I2C1PIRbits.PCIF,
                 (unsigned)I2C1ERRbits.NACKIF,
                 (unsigned)I2C1CON0,
                 (unsigned)I2C1CON1,
                 (unsigned)I2C1CON2,
                 (unsigned)I2C1CLK);
            __delay_ms(500);
            i2c_finish_transfer(I2C_ERROR_TIMEOUT);
            break;
        }

        wake_count++;
        SLEEP();
        NOP();
    }

    return s_i2c_transfer.status;
}

/// @brief Prepare for a new I2C transfer using the selected application handle.
static i2c_status_t i2c_begin_transfer(i2c_handle_t *handle, i2c_operation_t operation, uint16_t length)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    if (s_i2c_transfer.active || (s_active_handle != NULL))
    {
        return I2C_BUSY;
    }

    s_i2c_transfer.operation = operation;
    s_i2c_transfer.active = true;
    s_i2c_transfer.done = false;
    s_i2c_transfer.status = I2C_SUCCESS;
    s_i2c_transfer.length = length;
    s_active_handle = handle;

    I2C1PIR = 0x00U;
    I2C1ERR = 0x00U;
    PIR7bits.I2C1TXIF = 0U;
    PIR7bits.I2C1RXIF = 0U;
    PIR7bits.I2C1IF = 0U;
    PIR7bits.I2C1EIF = 0U;

    i2c_enable_module_events();

    LATDbits.LATD0 = 0U;
    LATDbits.LATD1 = 1U;
    IPR7bits.I2C1TXIP = 1U;
    IPR7bits.I2C1RXIP = 1U;
    IPR7bits.I2C1IP = 1U;
    IPR7bits.I2C1EIP = 1U;

    LATDbits.LATD3 = 1U;

    return I2C_SUCCESS;
}

/// @brief I2C1 Transmit Interrupt Service Routine
void __interrupt(irq(IRQ_I2C1TX), high_priority) I2C1_TX_ISR(void)
{
    if ((PIE7bits.I2C1TXIE == 0U) || (PIR7bits.I2C1TXIF == 0U))
    {
        return;
    }

    LATDbits.LATD2 ^= 1U;
    PIR7bits.I2C1TXIF = 0U;

    if ((!s_i2c_transfer.active) || (s_i2c_transfer.operation != I2C_OPERATION_WRITE) || (s_active_handle == NULL))
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

    if (s_active_handle->tx_pos < s_i2c_transfer.length)
    {
        I2C1TXB = s_active_handle->tx_buffer[s_active_handle->tx_pos];
        s_active_handle->tx_pos++;
    }
    else
    {
        PIE7bits.I2C1TXIE = 0U;
    }
}

/// @brief I2C1 Receive Interrupt Service Routine
void __interrupt(irq(IRQ_I2C1RX), high_priority) I2C1_RX_ISR(void)
{
    if ((PIE7bits.I2C1RXIE == 0U) || (PIR7bits.I2C1RXIF == 0U))
    {
        return;
    }

    LATDbits.LATD2 ^= 1U;
    PIR7bits.I2C1RXIF = 0U;

    if ((!s_i2c_transfer.active) || (s_i2c_transfer.operation != I2C_OPERATION_READ) || (s_active_handle == NULL))
    {
        return;
    }

    if (s_active_handle->rx_pos < s_i2c_transfer.length)
    {
        s_active_handle->rx_buffer[s_active_handle->rx_pos] = I2C1RXB;
        s_active_handle->rx_pos++;
    }

    if (s_active_handle->rx_pos >= s_i2c_transfer.length)
    {
        PIE7bits.I2C1RXIE = 0U;
    }
}

/// @brief I2C1 Event Interrupt Service Routine
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

// --- Public API ---
/// @brief Initialize the I2C1 hardware module master interface.
i2c_status_t I2C_Initialize(i2c_handle_t *handle, uint16_t speed_khz)
{
    if ((handle == NULL) || (speed_khz == 0U))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    I2C1CON0bits.EN = 0U;

    LATCbits.LATC3 = 1U;
    LATCbits.LATC4 = 1U;
    /* Configure the I2C pins as open-drain outputs per the device datasheet. */
    TRISCbits.TRISC3 = 0U;
    TRISCbits.TRISC4 = 0U;
    ANSELCbits.ANSELC3 = 0U;
    ANSELCbits.ANSELC4 = 0U;
    ODCONCbits.ODCC3 = 1U;
    ODCONCbits.ODCC4 = 1U;
    RC3I2Cbits.I2CPU = 0b10U;
    RC4I2Cbits.I2CPU = 0b10U;

    PPS_Unlock();
    RC3PPS = 0x37U;
    RC4PPS = 0x38U;
    I2C1SCLPPS = 0x13U;
    I2C1SDAPPS = 0x14U;
    PPS_Lock();

    handle->speed_khz = speed_khz;
    handle->retry_count = 3U;
    handle->tx_pos = 0U;
    handle->rx_pos = 0U;

    I2C1CLK = I2C1CLK_SRC_MFINTOSC;
    I2C1CON0bits.MODE = I2C_MODE_MASTER_7BIT;
    I2C1CON0bits.MDR = 1U;
    I2C1CON2bits.ABD = 0U;
    I2C1CON2bits.SDAHT = 0b01U;
    I2C1PIR = 0x00U;
    I2C1ERR = 0x00U;
    i2c_enable_module_events();
    I2C1CON0bits.EN = 1U;

    LATDbits.LATD1 = 1U;
    LATDbits.LATD2 = 1U;
    LATDbits.LATD3 = 1U;
    __delay_us(100);
    handle->initialized = true;

    return I2C_SUCCESS;
}

/// @brief Write the configured transmit buffer to the selected I2C slave.
i2c_status_t I2C_Write(i2c_handle_t *handle, uint8_t device_address, uint16_t length)
{
    i2c_status_t status;

    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    if ((handle->tx_buffer == NULL) || (handle->tx_buffer_size == 0U) || (length == 0U) || (length > handle->tx_buffer_size) || (length > 255U))
    {
        return I2C_ERROR_ILLEGAL_STATE;
    }

    status = i2c_begin_transfer(handle, I2C_OPERATION_WRITE, length);
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    handle->tx_pos = 0U;

    PIE7bits.I2C1RXIE = 0U;
    PIE7bits.I2C1TXIE = 1U;
    PIE7bits.I2C1IE = 1U;
    PIE7bits.I2C1EIE = 1U;

    I2C1ADB0 = device_address & 0xFEU;
    I2C1CNT = (uint8_t)length;
    I2C1TXB = handle->tx_buffer[handle->tx_pos];
    handle->tx_pos = 1U;
    LATDbits.LATD1 = 0U;
    I2C1CON0bits.S = 1U;
    if (I2C1CON0bits.S != 0U)
    {
        LATDbits.LATD2 = 0U;
    }

    return i2c_wait_transfer_complete();
}

/// @brief Read into the configured receive buffer from the selected I2C slave.
i2c_status_t I2C_Read(i2c_handle_t *handle, uint8_t device_address, uint16_t length)
{
    i2c_status_t status;

    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    if ((handle->rx_buffer == NULL) || (handle->rx_buffer_size == 0U) || (length == 0U) || (length > handle->rx_buffer_size) || (length > 255U))
    {
        return I2C_ERROR_ILLEGAL_STATE;
    }

    status = i2c_begin_transfer(handle, I2C_OPERATION_READ, length);
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    handle->rx_pos = 0U;

    PIE7bits.I2C1TXIE = 0U;
    PIE7bits.I2C1RXIE = 1U;
    PIE7bits.I2C1IE = 1U;
    PIE7bits.I2C1EIE = 1U;

    I2C1ADB0 = device_address | 0x01U;
    I2C1CNT = (uint8_t)length;
    I2C1CON0bits.S = 1U;
    if (I2C1CON0bits.S != 0U)
    {
        LATDbits.LATD2 = 0U;
    }

    LATDbits.LATD1 = 0U;
    return i2c_wait_transfer_complete();
}
