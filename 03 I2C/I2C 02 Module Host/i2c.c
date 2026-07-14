/* *****************************************************************************************
 *   File Name: i2clib.c
 *   Description: Hardware I2C library source file for PIC18 series Q43 microcontrollers.
 *   This library provides an interface for performing I2C operations, including reading
 *   and writing data as either a host or client device. The library is designed to be
 *   used with the I2C1 module provided by the PIC18___Q43, and it supports both 7-bit
 *   and 10-bit addressing modes.
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
 ***************************************************************************************** */
#include <xc.h>
#include <string.h>
#include "i2c.h"

#include "../../Libraries/PPSLIB/pps.h"

#ifndef _XTAL_FREQ
#define _XTAL_FREQ 64000000UL
#endif

static i2c_handle_t *active_handle = NULL; // Active handle for ISR access

/// @brief Clears all I2C interrupt flags.
/// @param None
/// @return None
static void i2c_clearInterruptFags(void)
{
    PIR7bits.I2C1RXIF = 0;
    PIR7bits.I2C1TXIF = 0;
    PIR7bits.I2C1IF = 0;
    PIR7bits.I2C1EIF = 0;
}

/// @brief Formats a 7-bit I2C address with the read/write bit.
/// @param device_address The 7-bit I2C device address.
/// @param read True for a read operation, false for a write operation.
/// @return The formatted 7-bit I2C address structure.
static i2c_address7_t format_7bit_address(uint8_t device_address, bool read)
{
    i2c_address7_t addr;
    addr.bits.value = device_address & 0x7F; // Ensure the address is 7 bits
    addr.bits.rw = read ? 1 : 0;             // Set the R/W bit based on the read parameter
    return addr;
}

/// @brief Formats a 10-bit I2C address with the read/write bit.
/// @param device_address The 10-bit I2C device address.
/// @param read True for a read operation, false for a write operation.
/// @return The formatted 10-bit I2C address structure.
static i2c_address10_t format_10bit_address(uint16_t device_address, bool read)
{
    i2c_address10_t addr;
    addr.address.bits.reserved = 0b11110;                     // Reserved bits
    addr.address.bits.value = (device_address >> 8U) & 0x03U; // High 2 address bits
    addr.address.bits.rw = read ? 1U : 0U;                    // Set the R/W bit based on the read parameter
    addr.address_l = (uint8_t)(device_address & 0xFFU);       // Get the low byte of the address
    return addr;
}

/// @brief Initializes the I2C handle with the specified mode and channel. This function
/// must be called before any I2C operations can be performed. It sets up the I2C
/// peripheral with the desired configuration, including the addressing mode and channel.
/// @param handle Pointer to the I2C handle structure.
/// @param channel The I2C channel to be used (if the microcontroller has more than one i2c
/// channel) as an ordinal 0, 1, 2, ...
/// @param mode The I2C mode to be used (e.g., host, client, multi-host).
/// @param speed The I2C clock speed in kHz. This parameter is used to calculate the
/// appropriate timing for I2C operations. The driver will compute the necessary clock
/// settings based on the provided speed and will select the appropriate clock source
/// for the I2C peripheral to achieve the desired communication speed.
/// @return The status of the I2C initialization, indicating success or any errors
i2c_status_t i2c_init(i2c_handle_t *handle, uint8_t channel, i2c_mode_t mode, uint16_t speed)
{
    if (handle == NULL)
    {
        return I2C_ERROR; // Handle pointer is null, cannot initialize
    }

    if (handle->initialized && handle->signature == I2C_HANDLE_SIGNATURE)
    {
        return I2C_ERROR_ALREADY_INITIALIZED; // Handle is already initialized, do not reinitialize
    }

    if (speed < 100 || speed > 5000)
    {
        return I2C_ERROR_INVALID_SPEED; // Invalid speed parameter, must be between 100 and 5000 kHz
    }

    memset(handle, 0x00, sizeof(i2c_handle_t)); // Clear the entire handle structure
    handle->mode = mode;
    handle->channel = channel;
    handle->speed_khz = speed;
    handle->signature = I2C_HANDLE_SIGNATURE;
    handle->current_operation = I2C_OP_NONE; // Set the initial operation state

    // Common hardware initialization regardless of mode
    I2C1CON0 = 0x00;        // Clear the control register to start with a known state
    TRISCbits.TRISC3 = 0;   // set to output pin
    TRISCbits.TRISC4 = 0;   // set to output pin
    ANSELCbits.ANSELC3 = 0; // Digital mode
    ANSELCbits.ANSELC4 = 0; // Digital mode
    ODCONCbits.ODCC3 = 1;   // Open-drain configuration
    ODCONCbits.ODCC4 = 1;   // Open-drain configuration

    PPS_Unlock();
    RC3PPS = 0x37;         // RC3 -> I2C1 SCL
    RC4PPS = 0x38;         // RC4 -> I2C1 SDA
    I2C1SCLPPS = 0b010011; // I2C1 SCL -> RC3
    I2C1SDAPPS = 0b010100; // I2C1 SDA -> RC4
    PPS_Lock();
    __delay_us(100); // Delay to allow the I2C lines to stabilize after configuration

    I2C1PIR = 0x00; // Clear all pending module events
    I2C1ERR = 0x00; // Clear all pending error conditions
    I2C1PIE = 0x00; // Leave module event interrupts disabled; TX/RX/error use top-level vectors

    i2c_clearInterruptFags();

    IPR7bits.I2C1RXIP = 1; // Match the vectored ISRs declared as high priority
    IPR7bits.I2C1TXIP = 1; // Match the vectored ISRs declared as high priority
    IPR7bits.I2C1IP = 1;   // Match the vectored ISRs declared as high priority
    IPR7bits.I2C1EIP = 1;  // Match the vectored ISRs declared as high priority
    PIE7bits.I2C1RXIE = 1; // Enable vectored RX interrupts
    PIE7bits.I2C1TXIE = 1; // TX interrupt advances each outgoing byte
    PIE7bits.I2C1IE = 0;   // General/event IRQ stays disabled for host byte progression
    PIE7bits.I2C1EIE = 1;  // Enable vectored error interrupts

    I2C1CON0bits.MODE = (uint8_t)mode;
    switch (mode)
    {
    // Configure the I2C peripheral for host mode
    case I2C_MODE_HOST_7BIT:
    case I2C_MODE_HOST_10BIT:

        // compute the neccessary clock source to use for the selected speed.
        // The Q43 family has multiple internal clock sources that can be used
        // for I2C.
        handle->speed_khz = speed;
        if (speed >= 400)
        {
            I2C1CLK = I2C_CLK_HFINTOSC; // Use HFINTOSC for higher speeds
            RC3I2Cbits.SLEW = 0b11;     // Use fast slew rate for higher speed operation
            RC3I2Cbits.I2CPU = 0b10;    // Use 10X pullups on RC3 for I2C SCL function
            RC3I2Cbits.I2CTH = 0b01;    // Use I2C standard thresholds for RC3
            RC4I2Cbits.SLEW = 0b11;     // Use fast slew rate for higher speed operation
            RC4I2Cbits.I2CPU = 0b10;    // Use 10X pullups on RC4 for I2C SDA function
            RC4I2Cbits.I2CTH = 0b01;    // Use I2C standard thresholds for RC4
        }
        else
        {
            I2C1CLK = I2C_CLK_MFINTOSC; // Use MFINTOSC for lower speeds
            RC3I2Cbits.SLEW = 0b01;     // Use slow slew rate for lower speed operation
            RC3I2Cbits.I2CPU = 0b01;    // Use 2X pullups on RC3 for I2C SCL function
            RC3I2Cbits.I2CTH = 0b01;    // Use I2C standard thresholds for RC3
            RC4I2Cbits.SLEW = 0b01;     // Use slow slew rate for lower speed operation
            RC4I2Cbits.I2CPU = 0b01;    // Use 2X pullups on RC4 for I2C SDA function
            RC4I2Cbits.I2CTH = 0b01;    // Use I2C standard thresholds for RC4
        }

        I2C1CON1 = 0x00;        // default everything else
        I2C1CON2 = 0x00;        // default everything else
        I2C1CON2bits.ABD = 0;   // Were using the address buffers
        I2C1CON2bits.SDAHT = 1; // Set SDA hold time to 300ns (T_scl/3) for standard mode timing
        break;

    case I2C_MODE_CLIENT_7BIT:
    case I2C_MODE_CLIENT_10BIT:
        // Configure the I2C peripheral for client mode
        // (e.g., set own address, enable client mode, etc.)
        break;

    default:
        handle->status = I2C_ERROR_ILLEGAL_STATE; // Invalid mode specified, handle error as needed
        return I2C_ERROR_ILLEGAL_STATE;
    }

    I2C1CON0bits.EN = 1; // Enable module after final CON0 mode configuration
    handle->initialized = true;
    return I2C_SUCCESS;
}

/// @brief Writes data to the specified I2C client device. This function initiates an
/// I2C write transaction to the given address, transmitting the specified data bytes.
/// The function will handle the necessary I2C protocol steps, including generating the
/// start condition, transmitting the address and data bytes, and generating the stop
/// condition. The status of the operation will be returned to indicate success or any
/// errors that may have occurred during the transaction.
/// @param handle Pointer to the I2C handle structure.
/// @param address The 7-bit I2C address of the target device.
/// @param data Pointer to the data buffer to be transmitted.
/// @param length Number of bytes to be transmitted.
/// @return The status of the I2C write operation.
i2c_status_t i2c_writeClient(i2c_handle_t *handle, uint16_t address, const uint8_t *data,
                             uint8_t length)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED; // Handle is not initialized, cannot perform write operation
    }

    if ((data == NULL) || (length == 0))
    {
        return I2C_ERROR_ILLEGAL_STATE; // Invalid data buffer or length, cannot perform write operation
    }

    // Track the users buffer and length in the handle for use in the ISR.
    // The actual transmission of data will be handled by the ISR.
    handle->tx_buffer = data;
    handle->tx_pos = 0;
    handle->current_operation = I2C_OP_WRITE; // Set the current operation to write

    // Initiate the I2C write transaction to the specified address
    // (e.g., generate start condition, send address with write bit, etc.)
    // The actual implementation of the I2C write transaction will depend on the
    // specific hardware and may involve setting registers and handling interrupts.
    active_handle = handle; // Set the active handle for use in the ISRs
    switch (handle->mode)
    {
    case I2C_MODE_HOST_7BIT:
    {
        i2c_address7_t addr7 = format_7bit_address((uint8_t)address, false); // Format the 7-bit address with
                                                                             // the write bit
        active_handle->device_address.address7 = addr7;                      // Store the formatted address
                                                                             // in the handle for ISR access
        I2C1ADB1 = addr7.address_l;                                          // Load the 7-bit address into ADB1,
                                                                             // ensuring the R/W bit is cleared
                                                                             // for write
        I2C1ADB0 = 0x00;                                                     // Clear ADB0 since we're using ADB1 for
                                                                             // the address in 7-bit host mode
        break;
    }
    case I2C_MODE_HOST_10BIT:
    {
        i2c_address10_t addr10 = format_10bit_address(address, false); // Format the 10-bit address with
                                                                       // the write bit
        active_handle->device_address.address10 = addr10;              // Store the formatted address in
                                                                       // the handle for ISR access
        I2C1ADB0 = addr10.address_l;                                   // Load the low byte of the 10-bit
                                                                       // address into ADB0
        I2C1ADB1 = addr10.address.address_h;                           // Load the high part of the 10-bit
                                                                       // address into ADB1
        break;
    }
    default:
        return I2C_ERROR_ILLEGAL_STATE;
    }
    I2C1CNT = (uint8_t)length;      // Load the byte count (not including address)
    I2C1TXB = handle->tx_buffer[0]; // Load the first byte of data to send into the TX buffer to prime the transfer
    handle->tx_pos = 1;             // Set the position for the next byte to send
    I2C1CON0bits.S = 1;             // Assert the start condition to begin the transfer

    return I2C_SUCCESS; // Return success status for now (actual implementation needed)
}

/// @brief Reads data from the specified I2C client device. This function initiates an
/// I2C read transaction from the given address, receiving the specified number of data
/// bytes. The function will handle the necessary I2C protocol steps, including generating
/// the start condition, transmitting the address with the read bit, receiving the data
/// bytes, and generating the stop condition. The received data will be stored in the
/// provided buffer, and the status of the operation will be returned to indicate success
/// or any errors that may have occurred during the transaction.
/// @param handle Pointer to the I2C handle structure.
/// @param address The 7-bit I2C address of the target device.
/// @param data Pointer to the buffer where the received data will be stored.
/// @param length Number of bytes to be received.
/// @return The status of the I2C read operation.
i2c_status_t i2c_readClient(i2c_handle_t *handle, uint16_t address, uint8_t *data,
                            uint8_t length)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED; // Handle is not initialized, cannot perform read operation
    }

    if ((data == NULL) || (length == 0))
    {
        return I2C_ERROR_ILLEGAL_STATE; // Invalid data buffer or length, cannot perform read operation
    }

    // Track the users buffer and length in the handle for use in the ISR.
    // The actual reception of data will be handled by the ISR.
    handle->rx_buffer = data;
    handle->rx_pos = 0;
    handle->current_operation = I2C_OP_READ; // Set the current operation to read

    // Initiate the I2C write transaction to the specified address
    // (e.g., generate start condition, send address with write bit, etc.)
    // The actual implementation of the I2C write transaction will depend on the
    // specific hardware and may involve setting registers and handling interrupts.
    active_handle = handle; // Set the active handle for use in the ISRs
    switch (handle->mode)
    {
    case I2C_MODE_HOST_7BIT:
    {
        i2c_address7_t addr7 = format_7bit_address((uint8_t)address, false); // Format the 7-bit address with the write bit
        active_handle->device_address.address7 = addr7;                      // Store the formatted address in the handle for ISR access
        I2C1ADB1 = addr7.address_l;                                          // Load the 7-bit address into ADB1, ensuring the R/W bit is cleared for write
        I2C1ADB0 = 0x00;                                                     // Clear ADB0 since we're using ADB1 for the address in 7-bit host mode
        break;
    }
    case I2C_MODE_HOST_10BIT:
    {
        i2c_address10_t addr10 = format_10bit_address(address, false); // Format the 10-bit address with the write bit
        active_handle->device_address.address10 = addr10;              // Store the formatted address in the handle for ISR access
        I2C1ADB0 = addr10.address_l;                                   // Load the low byte of the 10-bit address into ADB0
        I2C1ADB1 = addr10.address.address_h;                           // Load the high part of the 10-bit address into ADB1
        break;
    }
    default:
        return I2C_ERROR_ILLEGAL_STATE;
    }
    I2C1CNT = (uint8_t)length; // Load the byte count (not including address)
    I2C1CON0bits.S = 1;        // Assert the start condition to begin the transfer

    return I2C_SUCCESS; // Return success status for now (actual implementation needed)
}

/// @brief
/// @param handle
/// @param address
/// @param client_register
/// @param read_data
/// @param read_length
/// @return
i2c_status_t i2c_readClientRegister(i2c_handle_t *handle, uint16_t address, const uint8_t client_register,
                                    uint8_t *read_data, uint8_t read_length)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED; // Handle is not initialized, cannot perform write-read operation
    }

    if ((write_data == NULL) || (write_length == 0) ||
        (read_data == NULL) || (read_length == 0))
    {
        return I2C_ERROR_ILLEGAL_STATE; // Invalid data buffers or lengths, cannot perform write-read operation
    }

    // Track the user's write and read buffers and lengths in the handle for use in the ISR.
    // The actual transmission and reception of data will be handled by the ISR.
    handle->tx_buffer = write_data;
    handle->tx_buffer_size = write_length;
    handle->tx_buffer_pos = 0;
    handle->rx_buffer = read_data;
    handle->rx_buffer_size = read_length;
    handle->rx_buffer_pos = 0;
    handle->current_operation = I2C_OP_WRITE_READ; // Set the current operation state

    // Initiate the I2C write transaction to the specified address
    // (e.g., generate start condition, send address with write bit, etc.)
    // The actual implementation of the I2C write transaction will depend on the
    // specific hardware and may involve setting registers and handling interrupts.
    active_handle = handle; // Set the active handle for use in the ISRs
    switch (handle->mode)
    {
    case I2C_MODE_HOST_7BIT:
    {
        i2c_address7_t addr7 = format_7bit_address((uint8_t)address, false); // Format the 7-bit address with the write bit
        active_handle->device_address.address7 = addr7;                      // Store the formatted address in the handle for ISR access
        I2C1ADB1 = addr7.address_l;                                          // Load the 7-bit address into ADB1, ensuring the R/W bit is cleared for write
        I2C1ADB0 = 0x00;                                                     // Clear ADB0 since we're using ADB1 for the address in 7-bit host mode
        break;
    }
    case I2C_MODE_HOST_10BIT:
    {
        i2c_address10_t addr10 = format_10bit_address(address, false); // Format the 10-bit address with the write bit
        active_handle->device_address.address10 = addr10;              // Store the formatted address in the handle for ISR access
        I2C1ADB0 = addr10.address_l;                                   // Load the low byte of the 10-bit address into ADB0
        I2C1ADB1 = addr10.address.address_h;                           // Load the high part of the 10-bit address into ADB1
        break;
    }
    default:
        return I2C_ERROR_ILLEGAL_STATE;
    }
    I2C1CNT = (uint8_t)write_length; // Load the byte count (not including address)
    I2C1TXB = handle->tx_buffer[0];  // Load the first byte of data to send into the TX buffer to prime the transfer
    handle->tx_pos = 1;              // Set the position for the next byte to send
    I2C1CON0bits.S = 1;              // Assert the start condition to begin the transfer

    return I2C_SUCCESS; // Return success status for now (actual implementation needed)
}

/// @brief The I2C "General" interrupt service routine.
/// The host driver uses TX/RX/error top-level vectors for transfer pacing, so the
/// general/event path is left disabled during normal operation.
void __attribute__((weak)) __interrupt(irq(IRQ_I2C1), high_priority) i2c1_generalISR(void)
{
    I2C1PIR = 0x00;
    PIR7bits.I2C1IF = 0;
}

/// @brief The I2C "Error" interrupt service routine. This ISR is triggered when an error condition
/// occurs on the I2C bus, such as a NACK received, bus collision, or other error events as
/// indicated by the flags in the I2C Peripheral Interrupt Register (I2C1PIR) and the I2C Error
/// Register (I2C1ERR). The ISR will need to check the specific error flags to determine the
/// cause of the error and take appropriate action, such as clearing the error flags, updating
/// the transfer status, and signaling any waiting tasks or callbacks about the error condition.
void __attribute__((weak)) __interrupt(irq(IRQ_I2C1E), high_priority) i2c1_errorISR(void)
{

    if (active_handle == NULL || !active_handle->initialized)
    {
        PIR7bits.I2C1EIF = 0;
        return;
    }

    // Check for specific error conditions based on the flags in I2C1PIR and I2C1ERR
    // and take appropriate action (e.g., clear flags, update status, signal waiting tasks, etc.)
    if (I2C1ERRbits.NACKIF) // Check for NACK received error
    {
        I2C1ERRbits.NACKIF = 0;                          // Clear the NACK error flag
        active_handle->status = I2C_ERROR_NACK_RECEIVED; // Update the handle status to indicate a NACK error
        // Additional error handling actions can be taken here (e.g., signal waiting tasks, reset state, etc.)
    }
    if (I2C1ERRbits.BCLIF) // Check for bus collision error
    {
        I2C1ERRbits.BCLIF = 0;                           // Clear the bus collision error flag
        active_handle->status = I2C_ERROR_BUS_COLLISION; // Update the handle status to indicate a bus collision error
        // Additional error handling actions can be taken here (e.g., signal waiting tasks, reset state, etc.)
    }
    // Check for other error conditions as needed and handle them accordingly

    active_handle->current_operation = I2C_OP_NONE; // Clear the current operation state due to the error
    active_handle = NULL;                           // Clear the active handle since the transfer is now complete due to the error
    PIR7bits.I2C1EIF = 0;                           // Clear top-level error interrupt flag
    // The ISR will automatically stop being called once the transfer is complete and the handle
}

/// @brief The I2C "Transmit" interrupt service routine.
/// This ISR is triggered when the I2C module is ready to transmit the next byte of data on the
/// bus. The ISR will need to check the current operation state (e.g., host write, client
/// transmit, etc.) and load the next byte of data into the appropriate transmit register
/// (e.g., I2C1TXB) based on the user's buffer and the current position in the transfer. The
/// ISR may also need to check for conditions such as the byte count being complete, a stop
/// condition being detected, or an error condition occurring during transmission, and take
/// appropriate action based on those conditions.
void __attribute__((weak)) __interrupt(irq(IRQ_I2C1TX), high_priority) i2c1_transmitISR(void)
{
    if (active_handle == NULL || !active_handle->initialized ||
        (active_handle->current_operation != I2C_OP_WRITE && active_handle->current_operation != I2C_OP_WRITE_READ))
    {
        PIR7bits.I2C1TXIF = 0;
        return;
    }

    // Check if there are more bytes to transmit
    if (active_handle->tx_pos < active_handle->tx_buffer_size)
    {
        I2C1TXB = active_handle->tx_buffer[active_handle->tx_pos]; // Load the next byte to transmit
        active_handle->tx_pos++;                                   // Move to the next byte position
    }
    else
    {
        // Everything to transmit has been sent.  If we were in write_read mode, switch to
        // the read mode now to complete the repeated start condition and address phase for the read portion of the transfer.
        if (active_handle->current_operation == I2C_OP_WRITE_READ)
        {
            // We need to switch to read mode and reformat the address with the R/W bit set for read before the next transmission occurs in the ISR.
            switch (active_handle->mode)
            {
            case I2C_MODE_HOST_7BIT:
                active_handle->device_address.address7.bits.rw = 1;          // Set the R/W bit for read
                I2C1ADB1 = active_handle->device_address.address7.address_l; // Load the 7-bit address with the read bit into ADB1
                break;
            case I2C_MODE_HOST_10BIT:
                active_handle->device_address.address10.address.bits.rw = 1;          // Set the R/W bit for read
                I2C1ADB0 = active_handle->device_address.address10.address_l;         // Load low byte into ADB0
                I2C1ADB1 = active_handle->device_address.address10.address.address_h; // Load high byte with read bit into ADB1
                break;
            default:
                active_handle->status = I2C_ERROR_ILLEGAL_STATE;
                active_handle->current_operation = I2C_OP_NONE;
                active_handle = NULL;
                return;
            }
            I2C1CNT = (uint8_t)active_handle->rx_buffer_size; // Load the byte count for the read portion of the transfer
            active_handle->rx_pos = 0;                        // Reset the position for receiving bytes
            I2C1CON0bits.RSEN = 1;                            // Assert a repeated start condition to begin the read portion of the transfer
            active_handle->current_operation = I2C_OP_READ;   // Update state for receive ISR handling
        }
        else
        {
            // All bytes have been transmitted for a write-only operation, we can complete the transfer now.
            // STOP is generated by the hardware sequence as the transfer counter completes.
            active_handle->current_operation = I2C_OP_NONE; // Clear the current operation state
            active_handle = NULL;                           // Clear the active handle since the transfer is complete
            // The ISR will automatically stop being called once the transfer is complete and the handle
            // is cleared
        }
    }

    PIR7bits.I2C1TXIF = 0;
}

/// @brief The I2C "Receive" interrupt service routine.
/// This ISR is triggered when a byte has been received on the I2C bus and is available in the
/// receive buffer. The ISR will need to read the received byte from the appropriate register,
/// store it in the user's buffer, and update the transfer status accordingly. The ISR may also
/// need to check for conditions such as the byte count being complete, a stop condition being
/// detected, or an error condition occurring during reception, and take appropriate action
/// based on those conditions.
void __attribute__((weak)) __interrupt(irq(IRQ_I2C1RX), high_priority) i2c1_receiveISR(void)
{
    if (active_handle == NULL || !active_handle->initialized ||
        (active_handle->current_operation != I2C_OP_READ && active_handle->current_operation != I2C_OP_WRITE_READ))
    {
        PIR7bits.I2C1RXIF = 0;
        return;
    }

    // Check if there is space in the user's buffer to store the received byte
    if (active_handle->rx_pos < active_handle->rx_buffer_size)
    {
        active_handle->rx_buffer[active_handle->rx_pos] = I2C1RXB; // Read the received byte into the user's buffer
        active_handle->rx_pos++;                                   // Move to the next byte position
    }
    else
    {
        // Buffer overflow condition - more bytes received than the user's buffer can hold
        // Handle this condition as needed (e.g., discard additional bytes, set an error status, etc.)
        active_handle->status = I2C_ERROR_BUFFER_OVERFLOW; // Update the handle status to indicate a buffer overflow error
    }

    PIR7bits.I2C1RXIF = 0;
}