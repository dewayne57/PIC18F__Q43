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
#ifndef SPI_H
#define SPI_H
#include <stdbool.h>

/// @brief Defines the possible status codes returned by SPI functions.
typedef enum
{
    SPI_SUCCESS = 0,            // Operation completed successfully
    SPI_INVALID_ADDRESS,        // Address provided is out of range or not valid for the
                                // target device
    SPI_ERROR,                  // General error occurred
    SPI_ERROR_TIMEOUT,          // Operation timed out
    SPI_BUSY,                   // SPI module is busy
    SPI_ERROR_INVALID_PARAM,    // One or more parameters provided to the function are invalid
                                // (e.g. null pointer, zero length)
    SPI_ERROR_BUFFER_OVERFLOW,  // Received more data than the provided buffer can hold
    SPI_ERROR_BUFFER_UNDERFLOW, // Attempted to transmit more data than the provided buffer contains
    SPI_NOT_OPEN,               // SPI module is not open
    SPI_ALREADY_OPEN,           // SPI module is already open
    SPI_INVALID_STATE           // SPI module is in an invalid state
} spi_status_t;

/// @brief Defines the operating mode of the SPI module, either as a host (master) or
/// client (slave).
typedef enum
{
    SPI_CLIENT_MODE = 0, // SPI module operates as a slave, responding to the master's
                         // clock and data signals
    SPI_HOST_MODE = 1    // SPI module operates as a master, initiating communication
                         // and generating clock signals
} spi_mode_t;

/// @brief  Defines the bit order for SPI data transmission.
/// This enumeration is used to specify whether the SPI data should be transmitted
/// least significant bit (LSB) first or most significant bit (MSB) first.  The SPI
/// hardware module on the Q43 family devices supports both bit orders, and this setting
/// can be configured in the SPI_Init function.  Once set, it must not be changed during
/// an active SPI transaction, as this could lead to data corruption.
typedef enum
{
    MSB_FIRST = 0, // Default bit order for SPI communication
    LSB_FIRST = 1  // Alternative bit order for specific devices that require it
} spi_bitorder_t;

/// @brief Defines the type of SPI transfer being performed.  This setting can be used to
/// optimize the SPI communication for specific use cases, such as full-duplex data exchange,
/// transmitting data without reading (e.g. for write-only devices), or reading data without
/// transmitting (e.g. for devices that generate data without needing a write command).
typedef enum
{
    SPI_TRANSFER_FULL_DUPLEX = 0, // Data is exchanged simultaneously in both directions
                                  // (write and read at the same time)
    SPI_TRANSFER_TX_ONLY = 1,     // Write data to the SPI slave device without reading
                                  // any data back
    SPI_TRANSFER_RX_ONLY          // Read data from the SPI slave device without writing
                                  // any data (e.g. for devices that generate data without
                                  // needing a write command)
} spi_transfer_mode_t;

/// @brief  Defines the signal polarity desired for various SPI signals (e.g. clock polarity,
/// chip select polarity).  This setting can be used to configure the idle state of the
/// SPI signals.
typedef enum
{
    SPI_ACTIVE_HIGH = 0, // SPI signal is active high (idle state is low)
    SPI_ACTIVE_LOW = 1   // SPI signal is active low (idle state is high)
} spi_polarity_t;

typedef enum
{
    SPI_CLOCK_SOURCE_FOSC = 0b00000,     // Use the system clock (Fosc) as the SPI clock source
    SPI_CLOCK_SOURCE_HFINTOSC = 0b00001, // Use high frequence oscillator as the SPI clock source
    SPI_CLOCK_SOURCE_MFINTOSC = 0b00010, // Use medium frequency internal oscillator as the SPI clock source
    SPI_CLOCK_SOURCE_EXTOSC = 0b00011,   // Use external oscillator as the SPI clock source
    SPI_CLOCK_SOURCE_CLKREF = 0b00100    // Use the reference clock (CLKREF) as the SPI clock source
} spi_clock_source_t;

/// @brief  SPI module handle structure.  This structure can be used to store any necessary
///         state information for the SPI module.  For this demonstration, the SPI module
///         utilizes the hardware SPI module on the Q43 family devices AND interrupt-driven
///         processing for I/O.
typedef struct
{
    size_t clock_speed_khz;            // SPI clock speed in kHz (e.g. 1000 for 1 MHz)
    size_t clock_frequency_khz;        // Clock frequency in kHz based on the selected clock source and speed
    spi_clock_source_t clock_source;   // Clock source for the SPI module
    spi_bitorder_t bit_order;          // Bit order for SPI communication
    spi_mode_t mode;                   // Operating mode (host or client)
    spi_transfer_mode_t transfer_mode; // Type of SPI transfer (full-duplex, TX-only, RX-only)
    spi_polarity_t clock_polarity;     // Clock polarity (active high or active low)
    spi_polarity_t ss_polarity;        // Slave select polarity (active high or active low)
    spi_polarity_t input_polarity;     // Data input polarity (active high or active low)
    spi_polarity_t output_polarity;    // Data output polarity (active high or active low)

    // Application code should not alter or set any of the fields below this line.  These are
    // used internally by the SPI module implementation.
    spi_status_t status;    // Last recorded status of the SPI module (e.g., SPI_SUCCESS, SPI_ERROR, etc.)
    uint8_t *tx_buffer;     // Pointer to the SPI transmit buffer
    size_t tx_buffer_index; // Current index in the SPI transmit buffer for ongoing transactions
    size_t tx_buffer_size;  // Size of the SPI transmit buffer
    uint8_t *rx_buffer;     // Pointer to the SPI receive buffer
    size_t rx_buffer_index; // Current index in the SPI receive buffer for ongoing transactions
    size_t rx_buffer_size;  // Size of the SPI receive buffer
    bool initialized;       // Flag to indicate if the SPI module has been initialized
} spi_handle_t;

// API function prototypes
spi_status_t SPI_Open(spi_handle_t *handle);
spi_status_t SPI_Close(spi_handle_t *handle);
spi_status_t SPI_Write(spi_handle_t *handle, uint8_t address, uint8_t *data, size_t length);
spi_status_t SPI_Read(spi_handle_t *handle, uint8_t address, uint8_t *data, size_t length);
spi_status_t SPI_IsBusy(spi_handle_t *handle);
#endif /* SPI_H */