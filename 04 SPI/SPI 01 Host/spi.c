/* *****************************************************************************************
 *   File Name: spi.h
 *   Description: This file contains the SPI module settings for the demonstration project.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-09
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
 ***************************************************************************************** */
#include <xc.h>
#include <stdio.h>
#include <stdbool.h>
#include "config.h"
#include "spi.h"
#include "../../Libraries/UARTLIB/uartlib.h"
#include "../../Libraries/PPSLIB/pps.h"
#include "../../Libraries/INTLIB/intlib.h"

// Global variable to store the current SPI handle for use in ISRs
static spi_handle_t *current_handle;

/// @brief Validate the provided SPI device address against the expected range for the target
/// device(s).  This function can be used by the SPI_Write and SPI_Read functions to ensure
/// that the caller is attempting to communicate with a valid device address before initiating
/// an SPI transaction.  The valid address range may depend on the specific devices being
/// used in the demonstration, and this function can be updated accordingly to reflect the
/// expected address range and any reserved addresses that should not be used.
/// @param address SPI device address to validate.
/// @return Status indicating whether the address is valid (SPI_SUCCESS) or invalid
/// (SPI_INVALID_ADDRESS).
static spi_status_t validate_spi_address(uint8_t address)
{
    // For this demonstration, we will assume valid addresses are 0-7 (3 bits for device address)
    if (address > 7)
    {
        return SPI_INVALID_ADDRESS;
    }
    return SPI_SUCCESS;
}

/// @brief Clears all SPI1 interrupt flags.
/// @param None
/// @return None
static void clear_spi_interrupt_flags(void)
{
    PIR3bits.SPI1IF = 0;   // Clear SPI1 interrupt flag
    PIR3bits.SPI1RXIF = 0; // Clear SPI1 receive interrupt flag
    PIR3bits.SPI1TXIF = 0; // Clear SPI1 transmit interrupt flag
}

/// @brief Initializes the SPI module with the specified settings.
/// @param handle Pointer to the SPI handle structure containing the desired settings.
/// @return Status of the SPI initialization (e.g., SPI_SUCCESS, SPI_ERROR)
spi_status_t SPI_Open(spi_handle_t *handle)
{
    if (handle == NULL)
    {
        return SPI_ERROR_INVALID_PARAM;
    }
    if (handle->initialized)
    {
        return SPI_ALREADY_OPEN; // SPI module is already initialized
    }

    CRITICAL_SECTION_START();
    SPI1CON0bits.EN = 0; // Ensure SPI module is disabled before configuration

    // Keep SPI IRQ sources disabled until a transfer is started.
    // Enabling TXIE while idle can retrigger continuously on TXIF.
    PIE3bits.SPI1IE = 0;
    PIE3bits.SPI1RXIE = 0;
    PIE3bits.SPI1TXIE = 0;
    // PIE5 if SPI2 is being used for a second SPI interface in the future

    clear_spi_interrupt_flags(); // Clear SPI1 interrupt flags before enabling
    // interrupts to prevent an immediate interrupt after enabling
    IPR3bits.SPI1IP = 1;   // Set SPI1 interrupt priority to high
    IPR3bits.SPI1RXIP = 1; // Set SPI1 receive interrupt priority to high
    IPR3bits.SPI1TXIP = 1; // Set SPI1 transmit interrupt priority to high

    // Configure SPI1 module settings based on the handle parameters
    SPI1CON0bits.MST = (unsigned char)handle->mode;       // Set SPI mode (host/client)
    SPI1CON0bits.LSBF = (unsigned char)handle->bit_order; // Set bit order
    SPI1CON0bits.BMODE = 1;                               // Set bit length mode to full bytes (8 bits per frame)
    SPI1TWIDTH = 0;                                       // Set data width to 8 bits (0 = 8 bits, 1 = 16 bits)
                                                          //    SPI1CON0bits.TRMD = (unsigned char)handle->transfer_mode; // Set transfer mode

    // Set data input sample timing based on mode (master samples at the end of data output
    // time, slave samples at middle of data output time)
    SPI1CON1bits.SMP = (unsigned char)(handle->mode == SPI_HOST_MODE ? 1 : 0);
    SPI1CON1bits.CKE = 0;                                       // Set clock edge (data changes on transition from idle to active clock state)
    SPI1CON1bits.CKP = (unsigned char)handle->clock_polarity;   // Set clock polarity
    SPI1CON1bits.FST = 1;                                       // fast-start is enabled
    SPI1CON1bits.SSP = (unsigned char)handle->ss_polarity;      // Set slave select polarity (active low or active high)
    SPI1CON1bits.SDIP = (unsigned char)handle->input_polarity;  // Set data input polarity (active low or active high)
    SPI1CON1bits.SDOP = (unsigned char)handle->output_polarity; // Set data output polarity (active low or active high)

    SPI1CON2bits.BUSY = 0;  // Clear busy flag before enabling module
    SPI1CON2bits.SSFLT = 0; // clear slave select fault flag before enabling module
    SPI1CON2bits.SSET = 1;  // SPI module controls RC6 SS
    SPI1CON2bits.TXR = 1;   // Require Tx FIFO
    SPI1CON2bits.RXR = 1;   // Require Rx FIFO

    SPI1STATUS = 0; // Clear status register before enabling module

    // Set SPI clock source (e.g., Fosc/4, Fosc/16, etc.) based on handle parameter
    SPI1CLK = (unsigned char)handle->clock_source;
    // Set baud rate for desired clock speed (calculated based on the selected clock source and
    // desired speed in kHz)
    SPI1BAUD = (uint8_t)(((handle->clock_frequency_khz * 1000UL) /
                          (4UL * handle->clock_speed_khz * 1000UL)) -
                         1UL);

    // Set up interrupts we want to handle
    SPI1INTEbits.SRMTIE = 1; // Enable shift register empty interrupt (indicates when the last
    // bit of the last byte has been transmitted and the SPI bus is idle)
    SPI1INTEbits.TCZIE = 1; // Enable transfer counter zero interrupt (indicates when the
    // specified number of bytes have been transmitted/received)
    SPI1INTEbits.RXOIE = 1; // Enable receive overflow interrupt (indicates when the receive
    // FIFO has overflowed)
    SPI1INTEbits.TXUIE = 1; // Enable transmit underflow interrupt (indicates when the
    // transmit FIFO is empty)

    LATCbits.LATC0 = 1;   // Default address bit 0 high before enabling output
    LATCbits.LATC1 = 1;   // Default address bit 1 high before enabling output
    LATCbits.LATC2 = 1;   // Default address bit 2 high before enabling output
    TRISCbits.TRISC0 = 0; // RC0 is output (SPI device address bit 0)
    TRISCbits.TRISC1 = 0; // RC1 is output (SPI device address bit 1)
    TRISCbits.TRISC2 = 0; // RC2 is output (SPI device address bit 2)
    TRISCbits.TRISC3 = 0; // RC3 is output (SPI SCLK)
    TRISCbits.TRISC4 = 1; // RC4 is input (SPI SDI)
    TRISCbits.TRISC5 = 0; // RC5 is output (SPI SDO)
    TRISCbits.TRISC6 = 0; // RC6 is output (SPI SS)

    PPS_Unlock();
    SPI1SCKPPS = 0b010011; // SPI1 SCK input mapping (used in client mode)
    SPI1SDIPPS = 0b010100; // Map SPI1 SDI to RC4
    RC3PPS = 0x31;         // Map SPI1 SCK output to RC3
    RC5PPS = 0x32;         // Map SPI1 SDO to RC5
    SPI1SSPPS = 0b010110;  // Map SPI1 SS to RC6
    RC6PPS = 0x33;         // Map SPI1 SS to RC6
    PPS_Lock();

    handle->initialized = true;
    SPI1CON0bits.EN = 1; // Enable SPI module after configuration
    CRITICAL_SECTION_END();
    return SPI_SUCCESS;
}

/// @brief Close the SPI module and release any resources associated with it.
/// @param handle  Pointer to the SPI handle structure representing the SPI module to be closed.
/// @return Status of the SPI close operation.
spi_status_t SPI_Close(spi_handle_t *handle)
{
    if (handle == NULL)
    {
        return SPI_ERROR_INVALID_PARAM;
    }
    if (!handle->initialized)
    {
        return SPI_NOT_OPEN; // SPI module is not initialized
    }
    CRITICAL_SECTION_START();
    // Disable SPI module and SPI interrupts
    SPI1CON0bits.EN = 0;         // Disable SPI module
    PIE3bits.SPI1IE = 0;         // Disable SPI1 interrupt
    PIE3bits.SPI1RXIE = 0;       // Disable SPI1 receive interrupt
    PIE3bits.SPI1TXIE = 0;       // Disable SPI1 transmit interrupt
    clear_spi_interrupt_flags(); // Clear any pending SPI interrupts
    handle->initialized = false;
    CRITICAL_SECTION_END();
    return SPI_SUCCESS;
}

/// @brief Check if the SPI module is busy.
/// @param handle Pointer to the SPI handle structure representing the SPI module.
/// @return Status indicating whether the SPI module is busy (SPI_BUSY) or not
/// (SPI_SUCCESS).
spi_status_t SPI_IsBusy(spi_handle_t *handle)
{
    if (handle == NULL)
    {
        return SPI_ERROR_INVALID_PARAM;
    }
    if (!handle->initialized)
    {
        return SPI_NOT_OPEN; // SPI module is not initialized
    }
    if (SPI1CON2bits.SPI1BUSY)
    {
        return SPI_BUSY; // SPI module is currently busy with a transaction
    }
    return SPI_SUCCESS; // SPI module is not busy and ready for a new transaction
}

/// @brief Wait for the SPI transaction to complete.
/// @param handle Pointer to the SPI handle structure representing the SPI module.
/// @return Status of the wait operation (SPI_SUCCESS if the transaction completed successfully).
spi_status_t SPI_WaitForCompletion(spi_handle_t *handle)
{
    if (handle == NULL)
    {
        return SPI_ERROR_INVALID_PARAM;
    }
    if (!handle->initialized)
    {
        return SPI_NOT_OPEN; // SPI module is not initialized
    }
    while (SPI1CON2bits.SPI1BUSY)
    {
        // Wait for SPI transaction to complete
    }
    return SPI_SUCCESS;
}

/// @brief Write data to the SPI module.
/// @param handle Pointer to the SPI handle structure representing the SPI module.
/// @param address SPI device address to write to.
/// @param data Pointer to the data buffer to be written.
/// @param length Number of bytes to write.
/// @return Status of the SPI write operation.
spi_status_t SPI_Write(spi_handle_t *handle, uint8_t address, uint8_t *data, size_t length)
{
    if (handle == NULL || data == NULL || length == 0)
    {
        return SPI_ERROR_INVALID_PARAM;
    }
    if (!handle->initialized)
    {
        return SPI_NOT_OPEN; // SPI module is not initialized
    }
    spi_status_t address_status = validate_spi_address(address);
    if (address_status != SPI_SUCCESS)
    {
        return address_status; // Invalid address provided
    }

    current_handle = handle;         // Set the current handle for use in ISRs
    handle->tx_buffer = data;        // Set the transmit buffer pointer
    handle->tx_buffer_size = length; // Set the transmit buffer size
    SPI1CON2bits.RXR = 0;            // No receive, transmit only
    SPI1CON2bits.TXR = 1;            // Enable transmit
    SPI1CON2bits.SSET = 1;           // Explicitly enable SPI control of SS for this transfer

    // Load the address bits onto the appropriate pins (e.g., RC0-RC2 for a 3-bit address)
    LATCbits.LATC0 = address & 0x01;        // Set RC0 to address bit 0
    LATCbits.LATC1 = (address >> 1) & 0x01; // Set RC1 to address bit 1
    LATCbits.LATC2 = (address >> 2) & 0x01; // Set RC2 to address bit 2

    clear_spi_interrupt_flags();
    PIE3bits.SPI1RXIE = 0;
    PIE3bits.SPI1TXIE = 1;
    handle->tx_buffer_index = 1; // Initialize the transmit buffer index
    SPI1TCNT = length;           // Set transfer byte count for the SPI transaction
    SPI1TXB = data[0];           // Load the first byte of data into the transmit buffer to start the transaction

    handle->status = SPI_SUCCESS;
    return SPI_SUCCESS;
}

/// @brief Read data from the SPI module.
/// @param handle Pointer to the SPI handle structure representing the SPI module.
/// @param address SPI device address to read from.
/// @param data Pointer to the data buffer to store the read data.
/// @param length Number of bytes to read.
/// @return Status of the SPI read operation.
spi_status_t SPI_Read(spi_handle_t *handle, uint8_t address, uint8_t *data, size_t length)
{
    if (handle == NULL || data == NULL || length == 0)
    {
        return SPI_ERROR_INVALID_PARAM;
    }
    if (!handle->initialized)
    {
        return SPI_NOT_OPEN; // SPI module is not initialized
    }
    spi_status_t address_status = validate_spi_address(address);
    if (address_status != SPI_SUCCESS)
    {
        return address_status; // Invalid address provided
    }

    current_handle = handle;         // Set the current handle for use in ISRs
    handle->rx_buffer = data;        // Set the receive buffer pointer
    handle->rx_buffer_size = length; // Set the receive buffer size
    handle->rx_buffer_index = 0;     // Initialize the receive buffer index

    SPI1CON2bits.RXR = 1;  // Enable receive, no transmit
    SPI1CON2bits.TXR = 0;  // No transmit
    SPI1CON2bits.SSET = 1; // Explicitly enable SPI control of SS for this transfer

    // Load the address bits onto the appropriate pins (e.g., RC0-RC2 for a 3-bit address)
    LATCbits.LATC0 = address & 0x01;        // Set RC0 to address bit 0
    LATCbits.LATC1 = (address >> 1) & 0x01; // Set RC1 to address bit 1
    LATCbits.LATC2 = (address >> 2) & 0x01; // Set RC2 to address bit 2
    clear_spi_interrupt_flags();
    PIE3bits.SPI1TXIE = 0;
    PIE3bits.SPI1RXIE = 1;
    SPI1TCNT = length; // Set transfer byte count for the SPI transaction
    SPI1TXB = 0x00;    // Load dummy data to initiate the SPI transaction and \
                          generate clock pulses for the slave device to send \
                          data back
    handle->status = SPI_SUCCESS;
    return SPI_SUCCESS;
}

/// @brief Handle general SPI1 interrupts, which include shift register empty,
/// transfer counter zero, receive overflow, and transmit underflow conditions.  This ISR
/// should check the specific interrupt flags to determine the cause of the interrupt and
/// handle each condition appropriately (e.g., by loading the next byte to transmit, reading
/// received data, clearing overflow conditions, etc.).
void __attribute__((weak)) __interrupt(irq(IRQ_SPI1), high_priority) spi1_generalISR(void)
{
    // Handle shift register empty, transfer counter zero, receive overflow, and transmit underflow conditions
    if (SPI1INTEbits.SRMTIE && SPI1INTFbits.SRMTIF)
    {
        // Shift register empty interrupt handling (indicates SPI bus is idle after last bit of last byte has been transmitted)
        SPI1INTFbits.SRMTIF = 0; // Clear shift register empty interrupt flag
        // Handle any necessary actions when the SPI bus becomes idle after a transmission
    }
    if (SPI1INTEbits.TCZIE && SPI1INTFbits.TCZIF)
    {
        // Transfer counter zero interrupt handling (indicates specified number of bytes have been transmitted/received)
        SPI1INTFbits.TCZIF = 0; // Clear transfer counter zero interrupt flag
        PIE3bits.SPI1TXIE = 0;
        PIE3bits.SPI1RXIE = 0;
        // Handle any necessary actions when the specified number of bytes have been transferred
    }
    if (SPI1INTEbits.RXOIE && SPI1INTFbits.RXOIF)
    {
        // Receive overflow interrupt handling (indicates receive FIFO has overflowed)
        SPI1INTFbits.RXOIF = 0; // Clear receive overflow interrupt flag
        // Handle receive overflow condition (e.g., set error flag, discard data, etc.)
    }
    if (SPI1INTEbits.TXUIE && SPI1INTFbits.TXUIF)
    {
        // Transmit underflow interrupt handling (indicates transmit FIFO is empty when it should not be)
        SPI1INTFbits.TXUIF = 0; // Clear transmit underflow interrupt flag
        // Handle transmit underflow condition (e.g., set error flag, load next byte to transmit, etc.)
    }
}

/// @brief SPI1 receive interrupt service routine (ISR).  This ISR is triggered when a byte
/// of data is received by the SPI1 module.  The ISR should read the received data from the
/// SPI1 receive FIFO and store it in the appropriate location (e.g., a receive buffer
/// in the application code).
void __attribute__((weak)) __interrupt(irq(IRQ_SPI1RX), high_priority) spi1_receiveISR(void)
{
    if (current_handle == NULL)
    {
        PIE3bits.SPI1RXIE = 0;
        PIR3bits.SPI1RXIF = 0;
        return;
    }

    if (current_handle->rx_buffer_index >= current_handle->rx_buffer_size)
    {
        // Buffer overflow.  Discard the received byte and set an error flag or handle as needed.
        current_handle->status = SPI_ERROR_BUFFER_OVERFLOW; // Set an error flag or handle as needed
        PIE3bits.SPI1RXIE = 0;
        PIR3bits.SPI1RXIF = 0;
        return;
    }

    if (current_handle->rx_buffer_index >= current_handle->rx_buffer_size)
    {
        PIE3bits.SPI1RXIE = 0;
        PIR3bits.SPI1RXIF = 0;
        return;
    }

    // Read received byte from SPI1 receive buffer and store in receive buffer
    current_handle->rx_buffer[current_handle->rx_buffer_index++] = SPI1RXB;
}

/// @brief SPI1 transmit interrupt service routine (ISR).  This ISR is triggered when the SPI1
/// transmit buffer is empty and ready to accept the next byte of data to be transmitted.  The
/// ISR should load the next byte of data to be transmitted into the SPI1 transmit FIFO from
/// the appropriate location (e.g., a transmit buffer in the application code).
void __attribute__((weak)) __interrupt(irq(IRQ_SPI1TX), high_priority) spi1_transmitISR(void)
{
    if (current_handle == NULL)
    {
        PIE3bits.SPI1TXIE = 0;
        PIR3bits.SPI1TXIF = 0;
        return;
    }

    if (current_handle->tx_buffer_index >= current_handle->tx_buffer_size)
    {
        current_handle->status = SPI_SUCCESS;
        PIE3bits.SPI1TXIE = 0;
        PIR3bits.SPI1TXIF = 0;
        // Clear the transfer byte count to indicate that all bytes have been transmitted.
        SPI1TCNT = 0;
        return;
    }

    if (current_handle->tx_buffer_index >= current_handle->tx_buffer_size)
    {
        // No more data to transmit.  Disable transmit interrupt and clear flag.
        PIE3bits.SPI1TXIE = 0;
        PIR3bits.SPI1TXIF = 0;
    }
    else
    {
        // Load next byte to transmit from transmit buffer into SPI1 transmit buffer
        SPI1TXB = current_handle->tx_buffer[current_handle->tx_buffer_index++];
    }
}
