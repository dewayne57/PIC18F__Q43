/* *****************************************************************************************
 *   File Name: i2clib.c
 *   Description: Hardware I2C library source file for PIC18 series Q43 microcontrollers.
 *   This library provides an interface for performing I2C operations, including reading
 *   and writing data as either a master or slave device. The library is designed to be
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
#include "i2clib.h"

i2c_handle_t active_handle; // Global variable to hold the active I2C handle for ISR access

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
    addr.address = device_address & 0x7F; // Ensure the address is 7 bits
    addr.rw = read ? 1 : 0;              // Set the R/W bit based on the read parameter
    return addr;
}

/// @brief Formats a 10-bit I2C address with the read/write bit.
/// @param device_address The 10-bit I2C device address.
/// @param read True for a read operation, false for a write operation.
/// @return The formatted 10-bit I2C address structure.
static i2c_address10_t format_10bit_address(uint16_t device_address, bool read)
{
    i2c_address10_t addr;
    addr.master.reserved = 0b11110; // Reserved bits
    addr.master.address_h = (device_address >> 8) & 0x03; // Get the high 2 bits of the address
    addr.master.rw = read ? 1 : 0; // Set the R/W bit based on the read parameter
    addr.address_l = device_address & 0xFF; // Get the low byte of the address
    return addr;
}

/// @brief Initializes the I2C handle with the specified mode and channel. This function
/// must be called before any I2C operations can be performed. It sets up the I2C
/// peripheral with the desired configuration, including the addressing mode and channel.
/// @param handle Pointer to the I2C handle structure.
/// @param mode The I2C mode to be used (e.g., master, slave, multi-master).
/// @param channel The I2C channel to be used.
/// @param mode The I2C mode to be used (e.g., master, slave, multi-master).
/// @param speed The I2C clock speed in kHz. This parameter is used to calculate the
/// appropriate timing for I2C operations. The driver will compute the necessary clock
/// settings based on the provided speed and will select the appropriate clock source
/// for the I2C peripheral to achieve the desired communication speed.
/// @return The status of the I2C initialization, indicating success or any errors
i2c_status_t i2c_init(i2c_handle_t *handle, uint8_t channel, i2c_mode_t mode, uint16_t speed)
{
    if (handle == NULL)
    {
        return I2C_STATUS_ERROR; // Handle pointer is null, cannot initialize
    }

    if (handle->initialized && handle->signature == I2C_HANDLE_SIGNATURE)
    {
        return I2C_STATUS_ALREADY_INITIALIZED; // Handle is already initialized, do not reinitialize
    }
    if (speed < 100 || speed > 5000)
    {
        return I2C_STATUS_INVALID_SPEED; // Invalid speed parameter, must be between 100 and 5000 kHz
    }

    memset(handle, 0x00, sizeof(i2c_handle_t)); // Clear the entire handle structure
    handle->mode = mode;
    handle->channel = channel;
    handle->speed_khz = speed;
    handle->signature = I2C_HANDLE_SIGNATURE;
    handle->current_operation = I2C_OPERATION_NONE; // Set the initial operation state

    // Common hardware initialization regardless of mode
    I2C1CON0bits.EN = 0;    // Disable the module
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
    I2C1PIE = 0xDF; // Enable all interrups for the I2C module

    i2c_clearInterruptFags();

    IPR7bits.I2C1RXIP = 1; // Match the vectored ISRs declared as high priority
    IPR7bits.I2C1TXIP = 1; // Match the vectored ISRs declared as high priority
    IPR7bits.I2C1IP = 1;   // Match the vectored ISRs declared as high priority
    IPR7bits.I2C1EIP = 1;  // Match the vectored ISRs declared as high priority
    PIE7bits.I2C1RXIE = 1; // Enable vectored RX interrupts
    PIE7bits.I2C1TXIE = 1; // Enable vectored TX interrupts
    PIE7bits.I2C1IE = 1;   // Enable vectored event interrupts
    PIE7bits.I2C1EIE = 1;  // Enable vectored error interrupts
    I2C1CON0bits.EN = 1;   // Enable the module

    switch (mode)
    {
    // Configure the I2C peripheral for master mode
    // (e.g., set baud rate, enable master mode, etc.)
    case I2C_MODE_MASTER_7BIT:
    case I2C_MODE_MASTER_10BIT:

        // compute the neccessary clock source to use for the selected speed.
        // The Q43 family has multiple internal clock sources that can be used
        // for I2C.
        handle->speed_khz = speed_khz;
        if (speed >= 400)
        {
            I2C1CLK = I2C1CLK_SRC_HFINTOSC; // Use HFINTOSC for higher speeds
            RC3I2Cbits.SLEW = 0b11;         // Use fast slew rate for higher speed operation
            RC3I2Cbits.I2CPU = 0b10;        // Use 10X pullups on RC3 for I2C SCL function
            RC3I2Cbits.THR = 0b01;          // Use I2C standard thresholds for RC3
            RC4I2Cbits.SLEW = 0b11;         // Use fast slew rate for higher speed operation
            RC4I2Cbits.I2CPU = 0b10;        // Use 10X pullups on RC4 for I2C SDA function
            RC4I2Cbits.THR = 0b01;          // Use I2C standard thresholds for RC4
        }
        else
        {
            I2C1CLK = I2C1CLK_SRC_MFINTOSC; // Use MFINTOSC for lower speeds
            RC3I2Cbits.SLEW = 0b01;         // Use slow slew rate for lower speed operation
            RC3I2Cbits.I2CPU = 0b01;        // Use 2X pullups on RC3 for I2C SCL function
            RC3I2Cbits.THR = 0b01;          // Use I2C standard thresholds for RC3
            RC4I2Cbits.SLEW = 0b01;         // Use slow slew rate for lower speed operation
            RC4I2Cbits.I2CPU = 0b01;        // Use 2X pullups on RC4 for I2C SDA function
            RC4I2Cbits.THR = 0b01;          // Use I2C standard thresholds for RC4
        }

        I2C1CON0 = 0x00; // Set defaults and then configure master mode
        if (mode == I2C_MODE_MASTER_7BIT)
        {
            I2C1CON0bits.MODE = I2C_MODE_MASTER_7BIT; // 7-bit master mode
        }
        else
        {
            I2C1CON0bits.MODE = I2C_MODE_MASTER_10BIT; // 10-bit master mode
        }

        I2C1CON1 = 0x00;        // default everything else
        I2C1CON2 = 0x00;        // default everything else
        I2C1CON2bits.ABD = 0;   // Were using the address buffers
        I2C1CON2bits.SDAHT = 1; // Set SDA hold time to 300ns (T_scl/3) for standard mode timing
        break;

    case I2C_MODE_SLAVE_7BIT:
    case I2C_MODE_SLAVE_10BIT:
        // Configure the I2C peripheral for slave mode
        // (e.g., set own address, enable slave mode, etc.)
        break;

    case I2C_MODE_MULTI_MASTER_7BIT:
    case I2C_MODE_MULTI_MASTER_10BIT:
        // Configure the I2C peripheral for multi-master mode
        // (e.g., set baud rate, enable multi-master mode, etc.)
        break;

    default:
        handle->status = I2C_ERROR_ILLEGAL_STATE; // Invalid mode specified, handle error as needed
        return;
    }

    handle->initialized = true;
}

/// @brief Writes data to the specified I2C slave device. This function initiates an
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
i2c_status_t i2c_writeSlave(i2c_handle_t *handle, uint16_t address, const uint8_t *data,
                            uint8_t length)
{
    if ((handle == NULL) || (!handle->initialized))
    {
        return I2C_ERROR_NOT_INITIALIZED; // Handle is not initialized, cannot perform write operation
    }

    if ((data == NULL) || (length == 0) || (length > handle->buffer_size))
    {
        return I2C_ERROR_ILLEGAL_STATE; // Invalid data buffer or length, cannot perform write operation
    }

    // Track the users buffer and length in the handle for use in the ISR. 
    // The actual transmission of data will be handled by the ISR.
    handle->buffer = data;
    handle->buffer_size = length;
    handle->buffer_pos = 0;
    handle->current_operation = I2C_OP_WRITE; // Set the current operation to write

    // Initiate the I2C write transaction to the specified address
    // (e.g., generate start condition, send address with write bit, etc.)
    // The actual implementation of the I2C write transaction will depend on the
    // specific hardware and may involve setting registers and handling interrupts.
    active_handle = handle;                          // Set the active handle for use in the ISRs
    I2C1ADB0 = 0x00;                                 // Clear ADB0 since we're using ADB1 for the address in 7-bit master mode
    switch (handle->mode)
    {
    case I2C_MODE_MASTER_7BIT:
        i2c_address7_t addr7 = format_7bit_address((uint8_t)address, false); // Format the 7-bit address with the write bit
        I2C1ADB1 = addr7.address << 1; // Load the 7-bit address into ADB1, ensuring the R/W bit is cleared for write
        break;
    case I2C_MODE_MASTER_10BIT:
        i2c_address10_t addr10 = format_10bit_address(address, false); // Format the 10-bit address with the write bit
        I2C1ADB0 = addr10.address_l; // Load the low byte of the 10-bit address into ADB0
        I2C1ADB1 = (addr10.master.address_h << 1) | (addr10.master.rw); // Load the high part of the 10-bit address into ADB1, ensuring the R/W bit is cleared for write
        break;
    }
    I2C1CNT = (uint8_t)length;                       // Load the byte count (not including address)
    I2C1TXB = handle->tx_buffer[0];                  // Load the first byte of data to send into the TX buffer to prime the transfer
    handle->tx_pos = 1;                              // Set the position for the next byte to send
    handle->current_operation = I2C_OPERATION_WRITE; // Set the current operation state
    I2C1CON0bits.S = 1;                              // Assert the start condition to begin the transfer

    return I2C_STATUS_SUCCESS; // Return success status for now (actual implementation needed)
}

/// @brief The I2C "General" interrupt service routine. 
/// The "general" interrupt is triggered whenever any of the interrupt flags in the I2C Peripheral 
/// Interrupt Register (I2C1PIR) are set, indicating that an I2C event has occurred that requires 
/// attention, if the corresponding enable bit in the I2C Peripheral Interrupt Enable Register 
/// (I2C1PIE) is set, and if the global interrupt enable is set. This ISR will need to check the 
/// specific flags in I2C1PIR to determine the cause of the interrupt (e.g., byte count complete,
/// start condition detected, stop condition detected, error condition, etc.) and
void __attribute__((weak)) __interrupt(irq(IRQ_I2C1), high_priority) i2c1_generalISR(void)
{

}

/// @brief The I2C "Error" interrupt service routine. This ISR is triggered when an error condition 
/// occurs on the I2C bus, such as a NACK received, bus collision, or other error events as 
/// indicated by the flags in the I2C Peripheral Interrupt Register (I2C1PIR) and the I2C Error 
/// Register (I2C1ERR). The ISR will need to check the specific error flags to determine the 
/// cause of the error and take appropriate action, such as clearing the error flags, updating 
/// the transfer status, and signaling any waiting tasks or callbacks about the error condition.
void __attribute__((weak)) __interrupt(irq(IRQ_I2C1E), high_priority) i2c1_errorISR(void)
{

}

/// @brief The I2C "Transmit" interrupt service routine. 
/// This ISR is triggered when the I2C module is ready to transmit the next byte of data on the
/// bus. The ISR will need to check the current operation state (e.g., master write, slave 
/// transmit, etc.) and load the next byte of data into the appropriate transmit register 
/// (e.g., I2C1TXB) based on the user's buffer and the current position in the transfer. The 
/// ISR may also need to check for conditions such as the byte count being complete, a stop 
/// condition being detected, or an error condition occurring during transmission, and take 
/// appropriate action based on those conditions.
void __attribute__((weak)) __interrupt(irq(IRQ_I2C1TX), high_priority) i2c1_transmitISR(void)
{
    if (active_handle == NULL || !active_handle->initialized 
        || (active_handle->current_operation != I2C_OPERATION_WRITE && 
            active_handle->current_operation != I2C_OPERATION_WRITE_READ))
    {
        return;
    }

    // Check if there are more bytes to transmit
    if (active_handle->tx_pos < active_handle->buffer_size)
    {
        I2C1TXB = active_handle->buffer[active_handle->tx_pos]; // Load the next byte to transmit
        active_handle->tx_pos++; // Move to the next byte position
    }
    else
    {
        // All bytes have been loaded for transmission, wait for the STOP condition to complete the transfer
        active_handle->current_operation = I2C_OPERATION_NONE; // Clear the current operation state
        active_handle = NULL; // Clear the active handle since the transfer is complete
        // The ISR will automatically stop being called once the transfer is complete and the handle
        // is cleared
    }
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

}