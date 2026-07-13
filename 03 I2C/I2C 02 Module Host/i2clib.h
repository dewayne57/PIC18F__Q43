/* *****************************************************************************************
 *   File Name: i2clib.h
 *   Description: Hardware I2C library header file for PIC18 series Q43 microcontrollers.
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
 *   Usage Notes:
 *  - This library is designed to be used with the I2C1 module on PIC18F__Q43 family
 *    microcontrollers. It uses the hardware I2C module and is intended for applications
 *    that require efficient and reliable I2C communication.
 *
 *  - The user MUST define the _XTAL_FREQ macro in their project configuration to match
 *    the frequency of the external crystal oscillator used in their system. This is
 *    necessary for the delay functions to work correctly and to ensure proper timing
 *    of I2C operations.
 *
 ***************************************************************************************** */
#ifndef I2CLIB_H
#define I2CLIB_H

#include <stdint.h>
#include <stdbool.h>

// The maximum number of retries for an I2C operation before giving up and returning an error.
#define I2C_MAX_RETRIES 3
// The signature for a properly initialized I2C handle. This can be used to verify that a
// handle has been correctly initialized before performing I2C operations.
#define I2C_HANDLE_SIGNATURE 0xD42B

// Common clock sources for the I2C peripheral. The driver will select the appropriate
// clock source based on the desired I2C speed and the available system clocks.
#define I2C_CLK_EXTOSC 0b00101
#define I2C_CLK_REF 0b00100
#define I2C_CLK_HFINTOSC 0b00010
#define I2C_CLK_MFINTOSC 0b00011
#define I2C_CLK_FOSC 0b00001
#define I2C_CLK_FOSC_DIV4 0b00000

/// @brief Enumeration for I2C states. These states represent the various stages of an I2C
/// transaction, including idle, start condition, address transmission, data
/// transmission/reception, and stop condition. The driver will transition through these
/// states as it manages the I2C communication process.  The application code can use these
/// states to monitor the progress of I2C transactions and handle errors appropriately.
typedef enum
{
    I2C_IDLE = 0,              // No ongoing I2C transaction
    I2C_SUCCESS,               // I2C transaction completed successfully
    I2C_ERROR,                 // General I2C error
    I2C_ERROR_TIMEOUT,         // I2C transaction timed out
    I2C_ERROR_NOT_INITIALIZED, // I2C not initialized
    I2C_ERROR_ILLEGAL_STATE,   // Illegal I2C state
    I2C_ERROR_BUS_COLLISION,   // I2C bus collision detected
    I2C_ERROR_NACK_RECEIVED,   // Client returned NACK
    I2C_ERROR_BUFFER_OVERFLOW, // RX data exceeded caller-provided buffer
    I2C_ERROR_ALREADY_INITIALIZED,
    I2C_ERROR_INVALID_SPEED
} i2c_status_t;

/// @brief Enumeration for I2C operation types. This enumeration defines the possible
/// operations that can be performed on the I2C bus, including reading and writing data.
/// The driver will set the current operation to one of these values when an I2C transaction
/// is initiated, allowing the application code to determine the type of transaction in
/// progress and handle it accordingly.
typedef enum
{
    // Q43 I2C MODE field encodings (I2C1CON0.MODE)
    I2C_MODE_HOST_7BIT = 0b100,   // Host mode with 7-bit addressing
    I2C_MODE_HOST_10BIT = 0b101,  // Host mode with 10-bit addressing
    I2C_MODE_CLIENT_7BIT = 0b000, // Client mode with 7-bit addressing
    I2C_MODE_CLIENT_10BIT = 0b010 // Client mode with 10-bit addressing
} i2c_mode_t;

/// @brief Enumeration for I2C operation status. This enumeration defines the possible
/// statuses of an I2C operation, including no ongoing operation, read operation in
/// progress, and write operation in progress. The driver will update the current
/// operation status as it manages the I2C communication process, allowing the
/// application code to monitor the type of operation currently being performed on the
/// I2C bus.
typedef enum
{
    I2C_OP_NONE = 0,  // No ongoing operation
    I2C_OP_READ,      // Read operation in progress
    I2C_OP_WRITE,     // Write operation in progress
    I2C_OP_WRITE_READ // Combined write followed by read operation in progress
} i2c_operation_t;

/// @brief Typedef for representing a 10-bit I2C address, including the
/// read/write bit for host mode.
typedef struct
{
    union
    {
        struct
        {
            uint8_t reserved : 5; // Reserved bits for 10-bit addressing
            uint8_t value : 2;    // 10-bit I2C address high bit portion
            uint8_t rw : 1;       // read (not write) bit for 10-bit addressing
        } bits;
        uint8_t address_h; // the high byte of the 10-bit address, including the R/W bit
    } address;             // Union for the high byte of the 10-bit address, including the R/W bit
    uint8_t address_l;     // 10-bit I2C address low byte
} i2c_address10_t;

/// @brief Typedef for representing a 7-bit I2C address, including the
/// read/write bit for host mode. The 7-bit address is stored in the upper
/// 7 bits of the byte, and the least significant bit is used for the read/write
/// flag.
typedef union
{
    struct
    {
        uint8_t value : 7; // 7-bit I2C address
        uint8_t rw : 1;    // Read (1) or Write (0) bit for 7-bit addressing
    } bits;
    uint8_t address_l; // the low byte of the 10-bit address or the full 7-bit address
} i2c_address7_t;

/// @brief The i2c handle structure encapsulates all the necessary information
/// for managing an I2C peripheral. It includes pointers to the transmit and
/// receive buffers, their sizes, and the I2C operating mode. It also contains
/// internal state variables for tracking the current operation and status of
/// the I2C communication. This structure is designed to be used by the
/// application code to configure and manage I2C transactions, while the internal
/// fields are reserved for use by the driver implementation.
typedef struct
{
    i2c_mode_t mode;     // I2C operating mode
    uint8_t channel;     // I2C channel number (if multiple channels
                         // are supported, 1-based index)
    uint16_t speed_khz;  // I2C clock speed in kHz
    uint8_t retry_count; // Number of retries for the current operation

    // Everything below this line is for internal driver use and should not
    // be modified by the application.  It will be cleared and initialized
    // during the i2c_init() function processing.
    uint16_t signature;       // Unique signature to verify handle integrity
    uint16_t tx_buffer_size;  // Max size of the transmit buffer in bytes
    uint16_t rx_buffer_size;  // Max size of the receive buffer in bytes
    const uint8_t *tx_buffer; // Pointer to the transmit buffer
    uint8_t *rx_buffer;       // Pointer to the receive buffer
    uint8_t tx_buffer_pos;    // Position in the transmit buffer
    uint8_t rx_buffer_pos;    // Position in the receive buffer
    i2c_status_t status;      // I2C status

    union
    {
        i2c_address7_t address7;   // 7-bit I2C address structure
        i2c_address10_t address10; // 10-bit I2C address structure
    } device_address;              // Union for storing the device address in either 7-bit or 10-bit format

    volatile i2c_operation_t current_operation; // Current I2C operation (read/write/none)
    volatile uint8_t rx_pos;                    // Current position in the receive buffer
    volatile uint8_t tx_pos;                    // Current position in the transmit buffer

    bool initialized; // Flag to indicate if the handle has been initialized
} i2c_handle_t;

// Function prototypes for the I2C library

/// @brief Initializes the I2C handle with the specified mode and channel.
/// This function must be called before any I2C operations can be performed. It
/// configures the I2C peripheral according to the specified mode (e.g., host
/// or client) and selects the appropriate channel if multiple channels are supported.
/// @param handle Pointer to the I2C handle structure to be initialized.
/// @param channel The I2C channel number to be used (if multiple channels are
/// supported, this should be a 1-based index).
/// @param mode The I2C operating mode to be set for the handle (e.g
/// as host or client mode).
/// @param speed The I2C clock speed in kHz. This parameter is used to calculate
/// the appropriate timing for I2C operations. The driver will compute the necessary
/// clock settings based on the provided speed and will select the appropriate
/// clock source for the I2C peripheral to achieve the desired communication speed.
/// @return The status of the I2C initialization, indicating success or any errors
/// @note This function must be called before any other I2C operations are performed,
/// and the handle must be properly initialized to ensure correct operation of the I2C
/// driver.
i2c_status_t i2c_init(i2c_handle_t *handle, uint8_t channel, i2c_mode_t mode, uint16_t speed);

/// @brief Writes data to the specified I2C client device. This function initiates an
/// I2C write transaction to the given address, sending the specified data. The
/// function will handle the necessary I2C protocol steps, including generating the
/// start condition, transmitting the address and data bytes, and generating the
/// stop condition. The status of the operation will be returned to indicate success
/// or any errors that may have occurred during the transaction.
/// @param handle Pointer to the I2C handle structure.
/// @param address The 7-bit I2C address of the target device.
/// @param data Pointer to the data buffer to be transmitted.
/// @param length Number of bytes to be transmitted.
/// @return The status of the I2C write operation.
i2c_status_t i2c_writeClient(i2c_handle_t *handle, uint16_t address, const uint8_t *data,
                             uint8_t length);

/// @brief Reads data from the specified I2C client device. This function initiates an
/// I2C read transaction from the given address, receiving the specified amount of
/// data. The function will handle the necessary I2C protocol steps, including
/// generating the start condition, transmitting the address with the read bit set,
/// receiving the data bytes, and generating the stop condition. The status of the
/// operation will be returned to indicate success or any errors that may have
/// occurred during the transaction.
/// @param handle Pointer to the I2C handle structure.
/// @param address The 7-bit I2C address of the target device.
/// @param data Pointer to the data buffer to be received.
/// @param length Number of bytes to be received.
/// @return The status of the I2C read operation.
i2c_status_t i2c_readClient(i2c_handle_t *handle, uint16_t address, uint8_t *data,
                            uint8_t length);

/// @brief Performs a combined write followed by a read operation on the I2C bus.
/// This function is useful for client devices that require a register address to be
/// specified before reading data. The function will first perform a write
/// operation to send the register address or command, followed by a read operation
/// to receive the requested data. The function will handle the necessary I2C
/// protocol steps, including generating the start condition, transmitting the address
/// and data bytes for the write operation, generating a restart condition,
/// transmitting the address with the read bit set for the read operation, receiving
/// the data bytes, and generating the stop condition. The status of the operation
/// will be returned to indicate success or any errors that may have occurred during
/// the transaction.
/// @param handle Pointer to the I2C handle structure.
/// @param address The 7-bit I2C address of the target device.
/// @param write_data Pointer to the data buffer to be transmitted in the write phase.
/// @param write_length Number of bytes to be transmitted in the write phase.
/// @param read_data Pointer to the data buffer to be received in the read phase.
/// @param max_read_length Maximum number of bytes to be received in the read phase.
/// @param received_length Pointer to a variable where the actual number of bytes
/// received will be stored.
/// @return The status of the combined I2C write-read operation.
i2c_status_t i2c_writeReadClient(i2c_handle_t *handle, uint16_t address,
                                 const uint8_t *write_data, uint8_t write_length,
                                 uint8_t *read_data, uint8_t read_length);

/// @brief  Gets the current status of the I2C operation. This function allows
/// the application code to query the status of the I2C bus and determine if any
/// errors have occurred during the last operation. The status can indicate whether
/// the I2C transaction was successful, if there was a timeout, if the I2C bus is
/// idle, or if any other errors were detected. This information can be used by the
/// application to make informed decisions about how to proceed with further I2C
/// transactions or to handle error conditions appropriately.
/// @param handle Pointer to the I2C handle structure.
/// @return The current status of the I2C operation.
i2c_status_t i2c_getStatus(i2c_handle_t *handle);

/// @brief This function is used by client devices to send data back to the host
/// in response to a read request.
/// @param handle Pointer to the I2C handle structure.
/// @param data Pointer to the data buffer to be transmitted.
/// @param length Number of bytes to be transmitted.
/// @return The status of the I2C write operation.
i2c_status_t i2c_writeHost(i2c_handle_t *handle, const uint8_t *data,
                           uint8_t length);

/// @brief This function is used by client devices to receive data from the host
/// in response to a write request.
/// @param handle Pointer to the I2C handle structure.
/// @param data Pointer to the data buffer to be received.
/// @param max_length Maximum number of bytes to be received.
/// @param received_length Pointer to a variable where the actual number of
/// bytes received will be stored.
/// @return The status of the I2C read operation.
i2c_status_t i2c_readHost(i2c_handle_t *handle, uint8_t *data,
                          uint8_t max_length, uint8_t *received_length);

#endif // I2CLIB_H