/* *****************************************************************************************
 *   File Name: i2c.h
 *   Description: I2C hardware module interface for the demonstration project.
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
 *   This include file provides the data structures, constants, and function prototypes
 *   for the I2C hardware module interface.  The I2C module is used to communicate with
 *   external devices that support the I2C protocol, such as the MCP23017 I/O expander
 *   used in this demonstration project.  These functions are the basis for a reusable
 *   I2C library that can be used in other projects.
 ***************************************************************************************** */

#ifndef I2C_H
#define I2C_H

#include <stdbool.h>
#include <stdint.h>

/* *****************************************************************************************
 *   I2C data structures and enumerations
 ***************************************************************************************** */
#define I2C_HANDLE_SIGNATURE                                                                       \
    0x4943 // I2C handle signature (used to verify that the handle is valid)

// Define the I2C clock source enumeration.  The PIC18F47Q43 has several clock sources
// that can be used to drive the I2C module.  The I2C clock source enumeration is used to
// specify which clock source to use when calling the I2C functions.  The I2C clock source
// enumeration is defined as follows:
typedef enum
{
    I2C_CLOCK_FOSC_DIV_4 = 0, // FOSC/4
    I2C_CLOCK_FOSC = 1,       // FOSC
    I2C_CLOCK_HFINTOSC = 2,   // High frequency internal oscillator
    I2C_CLOCK_MFINTOSC = 3,   // Medium frequencey internal oscillator (500 kHz)
    I2C_CLOCK_CLKREF = 4,     // Clock reference (CLKREF)
    I2C_CLOCK_EXTREF = 5,     // External reference clock (EXTREF)
    I2C_CLOCK_TMR0 = 6,       // Timer 0 output clock (TMR0)
    I2C_CLOCK_TMR2_POST = 7,  // Timer 2 postscaler output clock (TMR2)
    I2C_CLOCK_TMR4_POST = 8,  // Timer 4 postscaler output clock (TMR4)
    I2C_CLOCK_TMR6_POST = 9,  // Timer 6 postscaler output clock (TMR6)
    I2C_CLOCK_SMT = 15,       // SMT clock (SMT)
    I2C_CLOCK_CLC1 = 16,      // CLC1 output clock (CLC1)
    I2C_CLOCK_CLC2 = 17,      // CLC2 output clock (CLC2)
    I2C_CLOCK_CLC3 = 18,      // CLC3 output clock (CLC3)
    I2C_CLOCK_CLC4 = 19,      // CLC4 output clock (CLC4)
    I2C_CLOCK_CLC5 = 20,      // CLC5 output clock (CLC5)
    I2C_CLOCK_CLC6 = 21,      // CLC6 output clock (CLC6)
    I2C_CLOCK_CLC7 = 22,      // CLC7 output clock (CLC7)
    I2C_CLOCK_CLC8 = 23       // CLC8 output clock (CLC8)
} I2C_Clock_t;

// Define the I2C status enumeration.  The I2C status enumeration is used to indicate the
// result of an I2C operation.  The I2C status enumeration is defined as follows:
typedef enum
{
    I2C_OK = 0,                     // I2C operation successful
    I2C_INVALID_HANDLE = 1,         // I2C operation failed due to invalid handle
    I2C_INVALID_MODE = 2,           // I2C operation failed due to invalid mode
    I2C_INVALID_CLOCK_SOURCE = 3,   // I2C operation failed due to invalid clock source
    I2C_INVALID_LENGTH = 4,         // I2C operation failed due to invalid length
    I2C_INVALID_TIMEOUT_VALUE = 5,  // I2C operation failed due to invalid timeout value
    I2C_INVALID_TIMEOUT_SOURCE = 6, // I2C operation failed due to invalid timeout source
    I2C_BUSY = 7,                   // I2C operation failed due to bus being busy
    I2C_BUS_TIMEOUT = 8,            // I2C bus is in timeout state
    I2C_BUS_COLLISION = 9,         // I2C bus is in collision state
    I2C_NACK_RECEIVED = 10, // I2C bus received a NAK (not acknowledge) from the slave device
    I2C_ERROR = 99          // I2C operation failed
} I2C_Status_t;

// Define the I2C address enumeration.  The I2C address enumeration is used to specify the
// address of the I2C device.  The I2C address enumeration is defined as follows:
typedef enum
{
    I2C_MODE_MASTER_7 = 4,  // I2C 7-bit master mode
    I2C_MODE_MASTER_10 = 5, // I2C 10-bit master mode
    I2C_MULTI_MASTER_7 = 6, // I2C 7-bit multi-master mode
    I2C_MULTI_MASTER_10 = 7 // I2C 10-bit multi-master mode
} I2C_Mode_t;

// Define the I2C address enumeration.  The I2C address enumeration is used to specify the
// address of the I2C device.  The I2C address enumeration is defined as follows:
typedef enum
{
    I2C_ADDRESS_7BIT = 0, // 7-bit addressing mode
    I2C_ADDRESS_10BIT = 1 // 10-bit addressing mode
} I2C_Address_t;

// Define the I2C bus state enumeration.  The I2C bus state enumeration is used to indicate
// the current state of the I2C bus.  The I2C bus state enumeration is defined as follows:
typedef enum
{
    I2C_IDLE = 0,      // I2C bus is idle
    I2C_WRITE,         // I2C bus is in write mode
    I2C_READ,          // I2C bus is in read mode
    I2C_READ_REGISTER, // I2C bus is in read register mode (the write phase). The read phase
                       // will be initiated after the write phase is complete by switching
                       // to I2C_READ mode.
    I2C_BUS_BUSY,      // I2C bus is busy
    I2C_BUS_ERROR      // I2C bus is in error state
} I2C_Bus_State_t;

/// @brief I2C timeout source enumeration.  The I2C timeout source enumeration is used to
/// specify the source of the timeout for I2C operations.  The I2C timeout source enumeration
/// is defined as follows:
typedef enum
{
    NONE = 0,   // No timeout source
    TIMER2 = 1, // Use Timer2 as the timeout source
    TIMER4 = 2, // Use Timer4 as the timeout source
    TIMER6 = 3, // Use Timer6 as the timeout source
    CLC1 = 7,   // Use CLC1 as the timeout source
    CLC2 = 8,   // Use CLC2 as the timeout source
    CLC3 = 9,   // Use CLC3 as the timeout source
    CLC4 = 10,  // Use CLC4 as the timeout source
    CLC5 = 11,  // Use CLC5 as the timeout source
    CLC6 = 12,  // Use CLC6 as the timeout source
    CLC7 = 13,  // Use CLC7 as the timeout source
    CLC8 = 14   // Use CLC8 as the timeout source
} I2C_Timeout_Source_t;

// Define the I2C handle structure.  The I2C handle structure is used to store the
// configuration and state of the I2C module.  The I2C handle structure is defined as follows:
typedef struct
{
    uint16_t signature;      // I2C handle signature (used to verify that the handle is valid)
    I2C_Clock_t clockSource; // I2C clock source
    I2C_Mode_t mode;         // I2C mode (master or slave)
    uint8_t* txBuffer; // Pointer to the transmit buffer (set when calling the I2C write function)
    uint8_t* rxBuffer; // Pointer to the receive buffer (set when calling the I2C read function)
    uint16_t txBufferSize;  // Size of the transmit buffer (set when calling the I2C write function)
    uint16_t rxBufferSize;  // Size of the receive buffer (set when calling the I2C read function)
    I2C_Bus_State_t state;  // I2C bus state (idle, write, read, or error)
    uint16_t txBufferIndex; // Current index in the transmit buffer
    uint16_t rxBufferIndex; // Current index in the receive buffer
    bool initialized;       // Flag indicating if the I2C module has been initialized
    I2C_Status_t lastStatus;            // last status of the I2C module (used for error handling)
    uint16_t timeout;                   // Timeout value for I2C operations (in milliseconds)
    I2C_Timeout_Source_t timeoutSource; // Timeout source for I2C operations (timer or CLC)
    uint16_t activeDeviceAddress;       // Active device address for current/next phase of transfer
    uint8_t registerAddressByte;        // Storage for one-byte register address write phase
} I2C_Handle_t;

// *****************************************************************************************
// I2C function prototypes
// *****************************************************************************************
I2C_Status_t I2C_GetLastStatus(I2C_Handle_t* handle);
I2C_Status_t I2C_Init(I2C_Handle_t* handle, I2C_Clock_t clockSource,
                      I2C_Mode_t mode, uint16_t timeout, I2C_Timeout_Source_t timeoutSource);
I2C_Status_t I2C_Write(I2C_Handle_t* handle, uint16_t deviceAddress, uint8_t* data,
                       uint16_t length);
I2C_Status_t I2C_Read(I2C_Handle_t* handle, uint16_t deviceAddress, uint8_t* data, uint16_t length);
I2C_Status_t I2C_ReadRegister(I2C_Handle_t* handle, uint16_t deviceAddress, uint8_t registerAddress,
                              uint8_t* data, uint16_t length);
I2C_Status_t I2C_Reset(I2C_Handle_t* handle);
I2C_Status_t I2C_IsBusy(I2C_Handle_t* handle);
#endif // I2C_H