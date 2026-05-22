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
 *   I2C1ADB1 (address), I2C1CNT (byte count), and I2C1CON0.S (start), then the I2C1 TX/RX
 *   and event/error ISRs complete the transaction.
 *
 *   Clock and timing notes:
 *   This project selects HFINTOSC as the I2C clock source via I2C1CLK.
 *   The bus is configured for Standard-mode operation (100 kHz).
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "../../Libraries/PPSLIB/pps.h"
#include "i2c.h"

#define I2C1CLK_SRC_HFINTOSC 0x02
#define I2C_MODE_MASTER_7BIT 0b100
#define I2C_START_TIMEOUT_CYCLES 60000U

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

/// @brief Pointer to the application-owned handle for the active transfer.
static i2c_handle_t *s_active_handle = NULL;

/// @brief Clear the top-level I2C interrupt flags. This is called at the start
/// of a transfer to ensure that any pending flags from previous activity are
/// cleared before enabling interrupts for the new transfer.
/// @param  None
/// @return None
static void i2c_clear_top_level_flags(void)
{
    PIR7bits.I2C1RXIF = 0U;
    PIR7bits.I2C1TXIF = 0U;
    PIR7bits.I2C1IF = 0U;
    PIR7bits.I2C1EIF = 0U;
}

/// @brief I2C1 Transmit Interrupt Service Routine
/// This ISR handles the transmission of data bytes during a write operation.  It checks
/// for the TX interrupt flag and the active transfer state, then loads the next byte of data
/// into the I2C1TXB register.  If a NACK is detected, it clears the NACK flag, sets the
/// transfer status to I2C_ERROR_NAK, and signals the transfer as done.  If all bytes
/// have been sent, it disables the TX interrupt and waits for the STOP condition to
/// complete the transfer.
void __interrupt(irq(IRQ_I2C1TX), high_priority) I2C1_TX_ISR(void)
{
    if ((PIE7bits.I2C1TXIE == 0U) || (PIR7bits.I2C1TXIF == 0U))
    {
        return;
    }
    if ((s_active_handle == NULL) || (s_active_handle->current_operation != I2C_OPERATION_WRITE))
    {
        return; // No active write transfer, ignore the interrupt
    }

    // If we're at the end of the data to send, disable the TX interrupt and wait
    // for STOP condition
    if (s_active_handle->tx_pos >= s_active_handle->tx_buffer_size || s_active_handle->tx_remaining <= 0)
    {
        PIE7bits.I2C1TXIE = 0U;                                  // Disable TX interrupt
        s_active_handle->current_operation = I2C_OPERATION_NONE; // Clear the current operation state
        s_active_handle = NULL;                                  // Clear the active handle to prevent further ISR activity
        return;
    }

    // Otherwise, load the next byte of data into the TX buffer to continue the transfer
    I2C1TXB = s_active_handle->tx_buffer[s_active_handle->tx_pos++];
    s_active_handle->tx_remaining--; // Decrement the remaining byte count
}

/// @brief I2C1 Receive Interrupt Service Routine
void __interrupt(irq(IRQ_I2C1RX), high_priority) I2C1_RX_ISR(void)
{
}

/// @brief I2C1 combined event interrupt service routine.
/// This handles module-level host events such as START, address, count and STOP completion.
void __interrupt(irq(IRQ_I2C1), high_priority) I2C1_EVENT_ISR(void)
{
    uint8_t pending_events;

    if ((PIE7bits.I2C1IE == 0U) || (PIR7bits.I2C1IF == 0U))
    {
        return;
    }

    pending_events = I2C1PIR;
    I2C1PIR = 0x00;
    PIR7bits.I2C1IF = 0U;

    if ((pending_events & 0x04U) != 0U)
    {
        if (s_active_handle != NULL)
        {
            s_active_handle->current_operation = I2C_OPERATION_NONE;
            s_active_handle = NULL;
        }
    }
}

/// @brief I2C1 Error Interrupt Service Routine
/// This ISR handles error conditions such as NACK and bus errors.  It also serves as a
/// catch-all for any unexpected conditions during an active transfer, ensuring that the
/// driver can recover gracefully and report the appropriate error status to the application.
void __interrupt(irq(IRQ_I2C1E), high_priority) I2C1_ERROR_ISR(void)
{
    if (s_active_handle == NULL)
    {
        return; // No active transfer, ignore the error
    }
    s_active_handle->current_operation = I2C_OPERATION_NONE; // Clear the current operation to prevent further ISR activity
    s_active_handle = NULL;                                  // Clear the active handle to prevent further ISR activity
}

// --- Public API ---
/// @brief Initialize the I2C1 hardware module master interface.
/// @Note this implementation uses the Q43 i2c peripheral that
/// is included in these family of controllers.  It is a departure
/// from the legacy MSSP-based peripherals and works somewhat
/// differently.
///
/// The module supports dedicated i2c pins with i2c specific pull
/// ups included, as well as DMA support, interrupts, and separate
/// registers.  If you're used to the legacy MSSP-based solutions,
/// this may be quite different.
///
/// The I2C pins are dedicated at RC3 (SCK) and RC4 (SDA).  It is
/// possible to remap these functions to alternate pins, but external
/// pull up resistors will be required.
i2c_status_t I2C_Initialize(i2c_handle_t *handle, uint16_t speed_khz)
{
    if ((handle == NULL) || (speed_khz == 0U))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    if (handle->initialized)
    {
        return I2C_SUCCESS;
    }

    I2C1CON0bits.EN = 0; // Disable the module

    /* Configure the I2C pins as open-drain outputs per the device datasheet. */
    TRISCbits.TRISC3 = 0;    // set to output pin
    TRISCbits.TRISC4 = 0;    // set to output pin
    ANSELCbits.ANSELC3 = 0;  // Digital mode
    ANSELCbits.ANSELC4 = 0;  // Digital mode
    ODCONCbits.ODCC3 = 1;    // Open-drain configuration
    ODCONCbits.ODCC4 = 1;    // Open-drain configuration
    RC3I2Cbits.I2CPU = 0b01; // Use 2X pullups on RC3 for I2C SCL function
    RC4I2Cbits.I2CPU = 0b10; // Use 2X pullups on RC4 for I2C SDA function

    PPS_Unlock();
    RC3PPS = 0x37;         // RC3 -> I2C1 SCL
    RC4PPS = 0x38;         // RC4 -> I2C1 SDA
    I2C1SCLPPS = 0b010011; // I2C1 SCL -> RC3
    I2C1SDAPPS = 0b010100; // I2C1 SDA -> RC4
    PPS_Lock();
    __delay_ms(100); // Delay to allow the I2C lines to stabilize after configuration

    // Initialize the handle state and I2C1 module registers for master operation.
    handle->speed_khz = speed_khz;
    handle->retry_count = 3;
    handle->tx_pos = 0;
    handle->rx_pos = 0;
    handle->tx_remaining = 0;
    handle->rx_remaining = 0;

    // Configure the I2C1 module for 7-bit master mode with the selected clock
    // source and timing.
    I2C1CLK = I2C1CLK_SRC_HFINTOSC;           // Select HFINTOSC as the clock source
    I2C1CON0 = 0x00;                          // Set defaults and then configure master mode
    I2C1CON0bits.MODE = I2C_MODE_MASTER_7BIT; // 7-bit master mode
    I2C1CON1 = 0x00;                          // default everything else
    I2C1CON2 = 0x00;                          // default everything else
    I2C1CON2bits.ABD = 0;                     // Were using the address buffers
    I2C1CON2bits.SDAHT = 1;                   // Set SDA hold time to 300ns (T_scl/3) for standard mode timing
    I2C1PIR = 0x00;                           // Clear all pending module events
    I2C1ERR = 0x00;                           // Clear all pending error conditions
    I2C1PIE = 0xDF;                           // Enable all interrups for the I2C module
    i2c_clear_top_level_flags();
    IPR7bits.I2C1RXIP = 1U; // Match the vectored ISRs declared as high priority
    IPR7bits.I2C1TXIP = 1U;
    IPR7bits.I2C1IP = 1U;
    IPR7bits.I2C1EIP = 1U;
    PIE7bits.I2C1RXIE = 1U; // Enable vectored RX interrupts
    PIE7bits.I2C1TXIE = 1U; // Enable vectored TX interrupts
    PIE7bits.I2C1IE = 1U;   // Enable vectored event interrupts
    PIE7bits.I2C1EIE = 1U;  // Enable vectored error interrupts
    I2C1CON0bits.EN = 1;    // Enable the module

    handle->current_operation = I2C_OPERATION_NONE; // Set the initial operation state
    handle->initialized = true;                     // Set the initialized flag last
    return I2C_SUCCESS;
}

/// @brief Write the configured transmit buffer to the selected I2C slave.
/// In 7-bit address mode, the device_address parameter is assumed to be a 8-bit
/// value with the least significant bit (LSB) reserved for the R/W bit.  The
/// driver will automatically clear the LSB for write operations, the caller need
/// not worry about this detail.  Also, the address is NOT shifted, the caller
/// should provide the 8-bit address value directly as likely documented in the
/// slave device datasheet.  For example, if the slave address is 0x50, the caller
/// should provide 0x50 for the device_address parameter, not 0x28.
///
/// In 7-bit master mode, the I2C1ADB0 is not used, the address is loaded into
/// the I2C1ADB1 register.  The first byte of the data is also initially loaded
/// into the I2C1TXB register to prime the module. The length of data to send
/// is loaded into the I2C1CNT register (not including the address byte). The
/// driver then asserts the start condition.  This causes the module to send
/// the start bit, followed by the address byte (with the R/W bit cleared for
/// write), and then the data byte from the I2C1TXB register.  Once the data
/// byte has been sent, the TXBE (Transmit Buffer Empty) event will occur, and
/// the ISR will load the next byte of data into the I2C1TXB register.  This process
/// continues until all bytes have been sent, at which point the ISR will disable
/// the TX interrupt and wait for the STOP condition to complete the transfer.
i2c_status_t I2C_Write(i2c_handle_t *handle, uint8_t device_address, uint16_t length)
{
    uint16_t start_timeout;

    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED;
    }

    if ((handle->tx_buffer == NULL) || (handle->tx_buffer_size == 0U) || (length == 0U) || (length > handle->tx_buffer_size) || (length > 255U))
    {
        return I2C_ERROR_ILLEGAL_STATE;
    }

    if ((I2C1CON0bits.EN == 0U) || (I2C1STAT0bits.BFRE == 0U) || (I2C1CON0bits.S != 0U) || (I2C1CON0bits.RSEN != 0U))
    {
        return I2C_BUSY;
    }

    I2C1PIR = 0x00;
    I2C1ERR = 0x00;
    i2c_clear_top_level_flags();

    s_active_handle = handle;                        // Set the active handle for use in the ISRs
    I2C1ADB0 = 0x00;                                 // Clear ADB0 since we're using ADB1 for the address in 7-bit master mode
    I2C1ADB1 = device_address & 0xFEU;               // Load the 7-bit address into ADB1, ensuring the R/W bit is cleared for write
    I2C1CNT = (uint8_t)length;                       // Load the byte count (not including address)
    I2C1TXB = handle->tx_buffer[0];                  // Load the first byte of data to send into the TX buffer to prime the transfer
    handle->tx_pos = 1;                              // Set the position for the next byte to send
    handle->current_operation = I2C_OPERATION_WRITE; // Set the current operation state
    handle->tx_remaining = length - 1;               // Set the remaining byte count for the ISRs to manage
    I2C1CON0bits.S = 1;                              // Assert the start condition to begin the transfer

    // If START never launches, the module can appear stalled with no PIR7 activity.
    start_timeout = I2C_START_TIMEOUT_CYCLES;
    while ((I2C1CON0bits.S != 0U) && (start_timeout > 0U))
    {
        start_timeout--;
    }

    if (start_timeout == 0U)
    {
        handle->current_operation = I2C_OPERATION_NONE;
        s_active_handle = NULL;
        return I2C_ERROR_TIMEOUT;
    }

    return I2C_SUCCESS;
}

/// @brief Read into the provided receive buffer from the selected I2C slave.
/// This function is not yet implemented in this example, but the intended operation
/// is as follows:
/// In 7-bit address mode, the device_address parameter is assumed to be a 8-bit
/// value with the least significant bit (LSB) reserved for the R/W bit.  The
/// driver will automatically set the LSB for read operations, the caller need
/// not worry about this detail.  Also, the address is NOT shifted, the caller
/// should provide the 8-bit address value directly as likely documented in the
/// slave device datasheet.  For example, if the slave address is 0x50, the caller
/// should provide 0x50 for the device_address parameter, not 0x28.
/// In 7-bit master mode, the I2C1ADB0 is not used, the address is loaded into
/// the I2C1ADB1 register.  The driver loads the I2C1CNT register with the number
/// of bytes to read.  The driver then asserts the start condition.  This causes
/// the module to send the start bit, followed by the address byte (with the R/W
/// bit set for read).  Once the address has been sent and acknowledged by the
/// slave, the module will automatically switch to receive mode and begin clocking
/// in data bytes from the slave.  Each time a byte is received, the RXIF event
/// will occur, and the ISR will read the byte from the I2C1RXB register and store
/// it in the application-provided receive buffer.  This process continues until
/// all requested bytes have been received, at which point the ISR will disable
/// the RX interrupt and wait for the STOP condition to complete the transfer.
/// The caller can check the status of the transfer by monitoring the handle's
/// current_operation field and the s_i2c_transfer state.  Once the transfer
/// is complete, the caller can check the status field for success or error
/// information.
/// @param handle Pointer to i2c_handle_t structure
/// @param device_address 8-bit I2C device address in read form; the driver toggles the low bit as needed
/// @param length Number of bytes to read from the I2C slave
/// @return i2c_status_t indicating success or error
i2c_status_t I2C_Read(i2c_handle_t *handle, uint8_t device_address, uint16_t length)
{
    return I2C_SUCCESS;
}
