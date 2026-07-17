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

#include <xc.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "config.h"
#include "i2c.h"
#include "../../Libraries/INTLIB/intlib.h"
#include "../../Libraries/PPSLIB/pps.h"

/// @brief Array of I2C handle structures for each channel.  This array is used to store the
/// configuration and state of the I2C module for each channel.  The array is indexed
/// by the I2C channel number (0 or 1).  The array is initialized to zero.  The array is
/// used to record the addresses of each handle so that the interrupt service routines
/// can update the appropriate buffers and handles. The array is defined as follows:
static I2C_Handle_t* i2cHandles[2]; // Array of I2C handle structures for each channel

// Forward definitions of all the internal functions that are not exposed to the user.
// These functions are used to validate the I2C handle structure and to perform the
// actual I2C read and write operations.  The functions are defined as follows:
static bool isHandleValid(I2C_Handle_t* handle);
static void cacheHandle(I2C_Handle_t* handle);
static void setupDeviceAddress(I2C_Handle_t* handle, uint16_t deviceAddress, bool read);
static void handleErrorInterrupt(I2C_Handle_t* handle);
static void handleGeneralInterrupt(I2C_Handle_t* handle);
static void handleReceiveInterrupt(I2C_Handle_t* handle);
static void handleTransmitInterrupt(I2C_Handle_t* handle);

#ifdef VECTORED_INTERRUPTS_ENABLED
#ifdef I2C1CON0
/// @brief I2C1 error interrupt service routine.
/// @param None
/// @return None
void __interrupt(irq(I2C1E), high_priority) I2C1_Error_ISR(void)
{
    I2C_Handle_t* handle = i2cHandles[0]; // Get the handle for I2C channel 1
    if (!isHandleValid(handle))
    {
        return; // Handle is not valid, exit the ISR
    }

    handleErrorInterrupt(handle); // Call the error interrupt handler
}

/// @brief I2C1 general interrupt service routine.
/// @param None
/// @return None
void __interrupt(irq(I2C1), high_priority) I2C1_General_ISR(void)
{
    I2C_Handle_t* handle = i2cHandles[0]; // Get the handle for I2C channel 1
    if (!isHandleValid(handle))
    {
        PIR7bits.I2C1IF = 0;
        return; // Handle is not valid, exit the ISR
    }

    handleGeneralInterrupt(handle); // Call the general interrupt handler
    PIR7bits.I2C1IF = 0;
}

/// @brief I2C1 Receive interrupt service routine.
/// @param None
/// @return None
void __interrupt(irq(I2C1RX), high_priority) I2C1_Receive_ISR(void)
{
    I2C_Handle_t* handle = i2cHandles[0]; // Get the handle for I2C channel 1
    if (!isHandleValid(handle))
    {
        return; // Handle is not valid, exit the ISR
    }

    handleReceiveInterrupt(handle); // Call the receive interrupt handler
}

/// @brief  I2C1 Transmit interrupt service routine.  This interrupt is triggered when the
/// data count > 0 and the transmit buffer is empty.  The interrupt service routine should
/// load the next byte of data into the transmit buffer.  If the data count is 0, the interrupt
/// service routine should disable the transmit interrupt and set the I2C bus state to idle.
/// @param None
/// @return None
void __interrupt(irq(I2C1TX), high_priority) I2C1_Transmit_ISR(void)
{
    I2C_Handle_t* handle = i2cHandles[0]; // Get the handle for I2C channel 1
    if (!isHandleValid(handle))
    {
        return; // Handle is not valid, exit the ISR
    }
    handleTransmitInterrupt(handle); // Call the transmit interrupt handler
}
#endif
#else
#ifdef I2C2CON0
/// @brief I2C2 error interrupt service routine.
/// @param None
/// @return None
void __interrupt(irq(I2C2E), high_priority) I2C2_Error_ISR(void)
{
    I2C_Handle_t* handle = i2cHandles[1]; // Get the handle for I2C channel 1
    if (!isHandleValid(handle))
    {
        return; // Handle is not valid, exit the ISR
    }

    handleErrorInterrupt(handle); // Call the error interrupt handler
}

/// @brief I2C2 Receive interrupt service routine.
/// @param None
/// @return None
void __interrupt(irq(I2C2RX), high_priority) I2C2_Receive_ISR(void)
{
    I2C_Handle_t* handle = i2cHandles[1]; // Get the handle for I2C channel 1
    if (!isHandleValid(handle))
    {
        return; // Handle is not valid, exit the ISR
    }

    handleReceiveInterrupt(handle); // Call the receive interrupt handler
}

/// @brief  I2C2 Transmit interrupt service routine.  This interrupt is triggered when the
/// data count > 0 and the transmit buffer is empty.  The interrupt service routine should
/// load the next byte of data into the transmit buffer.  If the data count is 0, the interrupt
/// service routine should disable the transmit interrupt and set the I2C bus state to idle.
/// @param None
/// @return None
void __interrupt(irq(I2C2TX), high_priority) I2C2_Transmit_ISR(void)
{
    I2C_Handle_t* handle = i2cHandles[1]; // Get the handle for I2C channel 1
    if (!isHandleValid(handle))
    {
        return; // Handle is not valid, exit the ISR
    }
    handleTransmitInterrupt(handle); // Call the transmit interrupt handler
}
#endif
#endif

/// @brief Get the last status code for the specified I2C handle.
/// @param handle Pointer to the I2C handle structure.
/// @return The last error code for the specified I2C handle.  If the handle is NULL, the
/// function returns NO_HANDLE.
I2C_Status_t I2C_GetLastStatus(I2C_Handle_t* handle)
{
    if (!isHandleValid(handle))
    {
        return I2C_INVALID_HANDLE; // Return an error code indicating that the handle is invalid
    }
    return handle->lastStatus; // Return the last state stored in the handle
}

/// @brief Initialize the I2C module.
/// @param handle Pointer to the I2C handle structure to be created by the init code.
/// @param channel I2C channel to be initialized (0 or 1).
/// @param clockSource I2C clock source to be used (FOSC/4, FOSC, HFINTOSC, MFINTOSC, CLKREF,
/// EXTREF, TMR0, TMR2, TMR4, TMR6, SMT, CLC1-8).
/// @param mode I2C mode to be used (master or slave).
/// @param timeout Timeout value for I2C operations (in milliseconds).  If zero, no timeout
/// will be used.  If non-zero, and a timeout source has been defined, the I2C module will
/// timeout if the operation takes longer than the specified timeout value.
/// @param timeoutSource Timeout source to be used for I2C operations (timer or CLC).  If no
/// timeout source is defined, the timeout parameter will be ignored.  If a timeout source
/// is defined, the I2C module will timeout if the operation takes longer than the specified
/// timeout value.
/// @return I2C status indicating success or failure.  The provided handle will be initialized
/// if the function returns I2C_STATUS_SUCCESS.  If the function returns I2C_STATUS_ERROR, the
/// handle will not be initialized and should not be used.
I2C_Status_t I2C_Init(I2C_Handle_t* handle, I2C_Channel_t channel, I2C_Clock_t clockSource,
                      I2C_Mode_t mode, uint16_t timeout, I2C_Timeout_Source_t timeoutSource)
{

    if (handle == NULL)
    {
        return I2C_INVALID_HANDLE; // Return an error code indicating that the handle is invalid
    }

    // Check if the handle has already been initialized
    if (isHandleValid(handle) && handle->initialized)
    {
        handle->lastStatus = I2C_OK; // Set the last state to OK
        return I2C_OK;               // Return success status if the handle is already initialized
    }

    memset(handle, 0, sizeof(I2C_Handle_t));  // Clear the handle structure
    handle->signature = I2C_HANDLE_SIGNATURE; // Set the handle signature to indicate that
                                              // the handle is valid

    // Validate the input parameters and set the last state in the handle accordingly
    if (mode != I2C_MODE_MASTER_7 && mode != I2C_MODE_MASTER_10 && mode != I2C_MULTI_MASTER_7 &&
        mode != I2C_MULTI_MASTER_10)
    {
        handle->lastStatus = I2C_INVALID_MODE; // Set the last state to invalid mode
        return I2C_INVALID_MODE; // Return an error code indicating that the mode is invalid
    }

    if (!((clockSource <= I2C_CLOCK_TMR6_POST) ||
          (clockSource >= I2C_CLOCK_SMT && clockSource <= I2C_CLOCK_CLC8)))
    {
        handle->lastStatus = I2C_INVALID_CLOCK_SOURCE; // Set the last state to invalid clock source
        return I2C_INVALID_CLOCK_SOURCE; // Return an error code indicating that the clock source is
                                         // invalid
    }

    if (channel == I2C_CHANNEL_1)
    {
#ifndef I2C1CON0
        handle->lastStatus = I2C_INVALID_CHANNEL; // Set the last state to invalid channel
        return I2C_INVALID_CHANNEL; // Return an error code indicating that the channel is invalid
#endif
    }
    if (channel == I2C_CHANNEL_2)
    {
#ifndef I2C2CON0
        handle->lastStatus = I2C_INVALID_CHANNEL; // Set the last state to invalid channel
        return I2C_INVALID_CHANNEL; // Return an error code indicating that the channel is invalid
#endif
    }
    if (channel != I2C_CHANNEL_1 && channel != I2C_CHANNEL_2)
    {
        handle->lastStatus = I2C_INVALID_CHANNEL; // Set the last state to invalid channel
        return I2C_INVALID_CHANNEL; // Return an error code indicating that the channel is invalid
    }

    // Now, set up the I2C structure and initialize the I2C hardware module based on the provided
    // parameters
    handle->channel = channel;             // Store the channel in the handle
    handle->clockSource = clockSource;     // Store the clock source in the handle
    handle->mode = mode;                   // Store the mode in the handle
    handle->timeout = timeout;             // Store the timeout value in the handle
    handle->timeoutSource = timeoutSource; // Store the timeout source in the handle
    cacheHandle(handle);

    // setup the I2C pins as open drain outputs
    TRISCbits.TRISC3 = 0; // RC3 is defined as output
    TRISCbits.TRISC4 = 0; // RC4 is output
    ODCONCbits.ODCC3 = 1; // Open Drain
    ODCONCbits.ODCC4 = 1; // Open drain

    // Set up the PPS (Peripheral Pin Select) so the module is routed to RC3/RC4.
    CRITICAL_SECTION_START();
    PPS_Unlock();
    I2C1SCLPPS = 0x13; // RC3 input mapping
    I2C1SDAPPS = 0x14; // RC4 input mapping
    RC3PPS = 0x37;     // I2C1 SCL output mapping
    RC4PPS = 0x38;     // I2C1 SDA output mapping
    PPS_Lock();
    CRITICAL_SECTION_END();

    RC3I2Cbits.I2CSLEW = 0b11; // Fast mode slew rate
    RC3I2Cbits.I2CPU = 0b10;   // 10X pullup
    RC3I2Cbits.I2CTH = 0b01;   // I2C thresholds
    RC4I2Cbits.I2CSLEW = 0b11;
    RC4I2Cbits.I2CPU = 0b10;
    RC4I2Cbits.I2CTH = 0b01;

    // If a timer timeout source was defined and a non-zero timeout value was provided, configure
    // the timer for the specified timeout value.  Note, the timeout value is in milliseconds, so
    // the timer must be configured to generate an interrupt after the specified number of
    // milliseconds.
    if ((timeoutSource == TIMER2 || timeoutSource == TIMER4 || timeoutSource == TIMER6) &&
        timeout > 0)
    {
        // configure the pre-scaller, post-scaler, and period registers for the system clock
        // speed (defined by _XTAL_FREQ) and the specified timeout value.  The timer will be
        // configured to generate an interrupt after the post scaler has counted the specified
        // number of timer overflows.
        uint32_t timerFrequency = _XTAL_FREQ / 4; // Timer frequency is FOSC/4
        uint8_t prescaler = 1;                    // Default prescaler value
        uint8_t postscaler = 1;                   // Default postscaler value
        uint32_t timerTicks =
            (timerFrequency / 1000) * timeout; // Calculate the number of timer ticks
        // The timer is an 8-bit counter, so the maximum number of ticks is 256.  If the number
        // of ticks is greater than 256, we need to use a pre-scaler and post-scaler to divide the
        // timer frequency.  The pre-scaler and post-scaler can be set to 1, 2, 4, 8, 16, 32, 64,
        // or 128.  We will use the smallest pre-scaler and post-scaler that will allow us to fit
        // the number of ticks into the 8-bit counter.
        uint32_t overflow = timerTicks / 256; // Calculate the number of overflows needed
        do
        {
            timerTicks = timerTicks / prescaler;  // Divide the number of ticks by the prescaler
            timerTicks = timerTicks / postscaler; // Divide the number of ticks by the postscaler

            overflow = timerTicks / 256; // Calculate the number of overflows needed
            if (overflow > 0)
            {
                if (prescaler < 128)
                {
                    prescaler *= 2; // Double the prescaler value
                }
                else if (postscaler < 16)
                {
                    postscaler++; // increment the postscaler value
                }
                else
                {
                    handle->lastStatus =
                        I2C_INVALID_TIMEOUT_VALUE;    // Set the last state to invalid timeout value
                    return I2C_INVALID_TIMEOUT_VALUE; // Return an error code indicating that the
                                                      // timeout value is invalid
                }
            }
        } while (overflow > 0);
        uint8_t preScaleExponent = 0;
        while (prescaler > 1)
        {
            prescaler /= 2;
            preScaleExponent++;
        }

        switch (timeoutSource)
        {
        case TIMER2:
            T2CONbits.TMR2ON = 0;                        // Disable Timer2 before configuring it
            T2CONbits.CKPS = (uint8_t)preScaleExponent;  // Set the Timer2 prescaler
            T2CONbits.OUTPS = (uint8_t)(postscaler - 1); // Set the Timer2 postscaler
            T2TMR = 0;
            T2PR = (uint8_t)timerTicks; // Set the Timer2 register to the calculated number of ticks
            T2HLT = 0;                  // Set the Timer2 hardware limit to 0 (no limit)
            T2HLTbits.PSYNC =
                1; // Set the Timer2 hardware limit to synchronize with the system clock
            T2CLKCONbits.T2CS = 0x01; // Set the Timer2 clock source to FOSC/4
            T2RST = 0;                // Set the Timer2 reset source to none
            break;
        case TIMER4:
            T4CONbits.TMR4ON = 0;                        // Disable Timer4 before configuring it
            T4CONbits.CKPS = (uint8_t)preScaleExponent;  // Set the Timer4 prescaler
            T4CONbits.OUTPS = (uint8_t)(postscaler - 1); // Set the Timer4 postscaler
            T4TMR = 0;
            T4PR = (uint8_t)timerTicks; // Set the Timer4 register to the calculated number of ticks
            T4HLT = 0;                  // Set the Timer4 hardware limit to 0 (no limit)
            T4HLTbits.PSYNC =
                1; // Set the Timer4 hardware limit to synchronize with the system clock
            T4CLKCONbits.T4CS = 0x01; // Set the Timer4 clock source to FOSC/4
            T4RST = 0;                // Set the Timer4 reset source to none
            break;
        case TIMER6:
            T6CONbits.TMR6ON = 0;                        // Disable Timer6 before configuring it
            T6CONbits.CKPS = (uint8_t)preScaleExponent;  // Set the Timer6 prescaler
            T6CONbits.OUTPS = (uint8_t)(postscaler - 1); // Set the Timer6 postscaler
            T6TMR = 0;
            T6PR = (uint8_t)timerTicks; // Set the Timer6 register to the calculated number of ticks
            T6HLT = 0;                  // Set the Timer6 hardware limit to 0 (no limit)
            T6HLTbits.PSYNC =
                1; // Set the Timer6 hardware limit to synchronize with the system clock
            T6CLKCONbits.T6CS = 0x01; // Set the Timer6 clock source to FOSC/4
            T6RST = 0;                // Set the Timer6 reset source to none
            break;
        default:
            handle->lastStatus =
                I2C_INVALID_TIMEOUT_SOURCE;    // Set the last state to invalid timeout source
            return I2C_INVALID_TIMEOUT_SOURCE; // Return an error code indicating that the timeout
                                               // source is invalid
        }
    }

    // If the clock source is the system oscillator (FOSC), the system clock / 4 (FOSC/4), or the
    // Hight Frequency Internal Oscillator (HFINTOSC), we will set the I2C fast mode.  Otherwise
    // we will leave it in normal mode.
    bool fastMode = false;
    if (clockSource == I2C_CLOCK_FOSC || clockSource == I2C_CLOCK_FOSC_DIV_4 ||
        clockSource == I2C_CLOCK_HFINTOSC)
    {
        fastMode = true; // Set the fast mode flag
    }

    if (handle->channel == I2C_CHANNEL_1)
    {
        CRITICAL_SECTION_START();
        I2C1CON0 = 0;                              // Reset I2C1 control register
        I2C1CON0bits.MODE = (uint8_t)handle->mode; // Set the I2C mode (master/slave)
        I2C1CON1 = 0;                              // Reset I2C1 control register
        I2C1CON2 = 0;                              // Reset I2C1 control register
        I2C1CON2bits.FME = fastMode ? 1 : 0;       // Set the I2C fast mode bit
        I2C1CLK = (uint8_t)handle->clockSource;    // Set the I2C clock source
        I2C1PIE = 0;                               // Disable I2C1 condition interrupts
        I2C1ERRbits.BTOIE = 1;                     // Enable I2C1 bus timeout interrupt
        I2C1ERRbits.BCLIE = 1;                     // Enable I2C1 bus collision interrupt
        I2C1ERRbits.NACKIE = 1;                    // Enable I2C1 NACK interrupt
        I2C1PIR = 0;                               // Clear I2C1 interrupt flags
        I2C1ERR = 0;                               // Clear I2C1 error flags
        I2C1CNT = 0;                               // Reset I2C1 count register
        if (handle->timeoutSource != NONE)
        {
            // Set the I2C1 bus timeout register to the specified timeout value
            I2C1BTO = (uint8_t)handle->timeout;
        }

        // Ensure I2C transfer pacing can preempt low-priority ISRs that may
        // initiate I2C operations (for example INT1 handler).
        IPR7bits.I2C1EIP = 1;
        IPR7bits.I2C1IP = 1;
        IPR7bits.I2C1TXIP = 1;
        IPR7bits.I2C1RXIP = 1;

        PIE7bits.I2C1EIE = 1;  // Enable I2C1 Error Interrupt Enable
        PIE7bits.I2C1TXIE = 1; // Enable I2C1 Transmit Interrupt Enable
        PIE7bits.I2C1RXIE = 1; // Enable I2C1 Receive Interrupt Enable
        PIE7bits.I2C1IE = 1;   // Enable I2C1 General Interrupt Enable
        CRITICAL_SECTION_END();

        I2C1CON0bits.EN = 1; // Enable the channel
    }
#ifdef I2C2CON0
    if (handle->channel == I2C_CHANNEL_2)
    {
        CRITICAL_SECTION_START();
        I2C2CON0 = 0;                              // Reset I2C2 control register
        I2C2CON0bits.MODE = (uint8_t)handle->mode; // Set the I2C mode (master/slave)
        I2C2CON1 = 0;                              // Reset I2C2 control register
        I2C2CON2 = 0;                              // Reset I2C2 control register
        I2C2CON2bits.FME = fastMode ? 1 : 0;       // Set the I2C fast mode bit
        I2C2CLK = (uint8_t)handle->clockSource;    // Set the I2C clock source
        I2C2PIE = 0;                               // Disable I2C2 interrupts
        I2C2PIR = 0;                               // Clear I2C2 interrupt flags
        I2C2ERRbits.BTOIE = 1;                     // Enable I2C2 bus timeout interrupt
        I2C2ERRbits.BCLIE = 1;                     // Enable I2C2 bus collision interrupt
        I2C2ERRbits.NACKIE = 1;                    // Enable I2C2 NACK interrupt
        I2C2CNT = 0;                               // Reset I2C2 count register

        IPR6bits.I2C2EIP = 1;
        IPR6bits.I2C2IP = 1;
        IPR6bits.I2C2TXIP = 1;
        IPR6bits.I2C2RXIP = 1;

        PIE6bits.I2C2EIE = 1;  // Enable I2C2 Error Interrupt Enable
        PIE6bits.I2C2TXIE = 1; // Enable I2C2 Transmit Interrupt Enable
        PIE6bits.I2C2RXIE = 1; // Enable I2C2 Receive Interrupt Enable
        PIE6bits.I2C2IE = 1;   // Enable I2C2 General Interrupt Enable
        CRITICAL_SECTION_END();

        I2C2CON0bits.EN = 1; // Enable the channel
    }
#endif
    __delay_ms(1);               // Short delay to let the module come up
    handle->initialized = true;  // Mark the handle as initialized
    handle->lastStatus = I2C_OK; // Set the last state to OK
    return I2C_OK;               // Return success status
}

/// @brief Write data to an I2C device.
/// @param handle Pointer to the I2C handle structure.
I2C_Status_t I2C_Write(I2C_Handle_t* handle, uint16_t deviceAddress, uint8_t* data, uint16_t length)
{
    if (!isHandleValid(handle))
    {
        return I2C_INVALID_HANDLE; // Return an error code indicating that the handle is invalid
    }
    I2C_Status_t status = I2C_OK;
    if ((status = I2C_IsBusy(handle)) != I2C_OK)
    {
        return status;
    }
    if (length <= 0)
    {
        handle->lastStatus = I2C_INVALID_LENGTH; // Set the last state to invalid length
        return I2C_INVALID_LENGTH; // Return an error code indicating that the length is invalid
    }
    cacheHandle(handle);
    handle->activeDeviceAddress = deviceAddress;

    handle->txBuffer = data;       // Set the transmit buffer pointer
    handle->txBufferSize = length; // Set the transmit buffer size
    handle->txBufferIndex = 0;     // Reset the transmit buffer index

    setupDeviceAddress(handle, deviceAddress, false); // Setup the device address
    PIE7bits.I2C1RXIE = 0;                            // TX transfer only
    PIE7bits.I2C1TXIE = 1;                            // Enable the transmit interrupt
    handle->state = I2C_WRITE;                        // Set the I2C bus state to write mode
    I2C1CNT = (uint8_t)length; // Set the I2C count register to the number of bytes to transmit
    I2C1CON0bits.RSEN = 0;     // Disable repeated start condition
    I2C1CON0bits.CSTR = 0;     // Release clock stretching before starting the transfer
    I2C1CON0bits.S = 1;        // Generate a start condition

    handle->lastStatus = I2C_OK; // Set the last state to OK
    return I2C_OK; // Return success status (actual write implementation is not provided)
}

/// @brief Read data from an I2C device.
/// @param handle Pointer to the I2C handle structure.
/// @param deviceAddress The I2C address of the device to read from.
/// @param data Pointer to the buffer to store the read data.
/// @param length The number of bytes to read from the device.
/// @return I2C status indicating success or failure.  The provided handle will be updated
/// with the read data if the function returns I2C_STATUS_SUCCESS.  If the function returns
/// I2C_STATUS_ERROR, the handle will not be updated and should not be used.
I2C_Status_t I2C_Read(I2C_Handle_t* handle, uint16_t deviceAddress, uint8_t* data, uint16_t length)
{
    if (!isHandleValid(handle))
    {
        return I2C_INVALID_HANDLE; // Return an error code indicating that the handle is invalid
    }
    I2C_Status_t status = I2C_OK;
    if ((status = I2C_IsBusy(handle)) != I2C_OK)
    {
        return status;
    }
    if (length <= 0)
    {
        handle->lastStatus = I2C_INVALID_LENGTH; // Set the last state to invalid length
        return I2C_INVALID_LENGTH; // Return an error code indicating that the length is invalid
    }
    cacheHandle(handle);
    handle->activeDeviceAddress = deviceAddress;

    handle->rxBuffer = data;       // Set the receive buffer pointer
    handle->rxBufferSize = length; // Set the receive buffer size
    handle->rxBufferIndex = 0;     // Reset the receive buffer index

    setupDeviceAddress(handle, deviceAddress, true); // Setup the device address for read operation
    PIE7bits.I2C1TXIE = 0;                           // RX transfer only
    PIE7bits.I2C1RXIE = 1;                           // Enable the receive interrupt
    handle->state = I2C_READ;                        // Set the I2C bus state to read mode
    I2C1CNT = (uint8_t)length; // Set the I2C count register to the number of bytes to receive
    I2C1CON0bits.RSEN = 0;     // Disable repeated start condition
    I2C1CON0bits.CSTR = 0;     // Release clock stretching before starting the transfer
    I2C1CON0bits.S = 1;        // Generate a start condition

    handle->lastStatus = I2C_OK; // Set the last state to OK
    return I2C_OK; // Return success status (actual read implementation is not provided)
}

/// @brief Read data from a specific register of an I2C device.  This function is used to read
/// data from a specific register of an I2C device.  The function must first write the register
/// address as a single byte to the define (with the address of the device and the r/w bit cleared).
/// It then generates a repeated start condition and reads the specified number of bytes from the
/// device.
/// @param handle Pointer to the I2C handle structure.
/// @param deviceAddress The I2C address of the device to read from.
/// @param registerAddress The register address to read from.
/// @param data Pointer to the buffer to store the read data.
/// @param length The number of bytes to read from the device.
/// @return I2C status indicating success or failure.  The provided handle will be updated
/// with the read data if the function returns I2C_OK.
I2C_Status_t I2C_ReadRegister(I2C_Handle_t* handle, uint16_t deviceAddress, uint8_t registerAddress,
                              uint8_t* data, uint16_t length)
{
    if (!isHandleValid(handle))
    {
        return I2C_INVALID_HANDLE; // Return an error code indicating that the handle is invalid
    }
    I2C_Status_t status;
    if ((status = I2C_IsBusy(handle)) != I2C_OK)
    {
        return status;
    }
    if (length <= 0)
    {
        handle->lastStatus = I2C_INVALID_LENGTH; // Set the last state to invalid length
        return I2C_INVALID_LENGTH; // Return an error code indicating that the length is invalid
    }

    cacheHandle(handle);
    handle->activeDeviceAddress = deviceAddress;
    handle->lastStatus = I2C_OK;

    // Prepare receive context for the second phase (read after repeated-start).
    handle->rxBuffer = data;
    handle->rxBufferSize = length;
    handle->rxBufferIndex = 0;

    // Phase 1: address + write + one register byte.
    // Phase 2 (repeated-start + read) is initiated by the I2C general ISR
    // when the hardware byte count reaches zero.
    handle->registerAddressByte = registerAddress;
    handle->txBuffer = &handle->registerAddressByte;
    handle->txBufferSize = 1;
    handle->txBufferIndex = 1;

    setupDeviceAddress(handle, deviceAddress, false);
    PIE7bits.I2C1TXIE = 1;
    PIE7bits.I2C1RXIE = 0;
    handle->state = I2C_READ_REGISTER;
    I2C1CNT = 1;
    I2C1TXB = handle->registerAddressByte;
    I2C1CON0bits.RSEN = 0;
    I2C1CON0bits.CSTR = 0;
    I2C1CON0bits.S = 1;

    handle->lastStatus = I2C_OK;
    return I2C_OK;
}

/// @brief Reset the I2C module.  This function resets the I2C module by clearing all interrupts
/// and resetting any operations in progress.  The function is defined as follows:
I2C_Status_t I2C_Reset(I2C_Handle_t* handle)
{
    if (!isHandleValid(handle))
    {
        return I2C_INVALID_HANDLE; // Return an error code indicating that the handle is invalid
    }
    cacheHandle(handle);

    // Reset the I2C module based on the channel
#ifdef I2C1CON0
    if (handle->channel == I2C_CHANNEL_1)
    {
        CRITICAL_SECTION_START();
        PIE7bits.I2C1TXIE = 0; // Stop byte-pacing interrupts during reset
        PIE7bits.I2C1RXIE = 0;
        I2C1CON0bits.CSTR = 0; // Release clock stretching if active
        I2C1CON0bits.RSEN = 0; // Cancel any repeated-start request
        I2C1CON0bits.S = 0;    // Clear the start condition bit
        I2C1CON0bits.EN = 0;   // Disable the I2C module
        PIR7bits.I2C1EIF = 0;  // Clear the I2C1 error interrupt flag
        PIR7bits.I2C1IF = 0;   // Clear the I2C1 general interrupt flag
        PIR7bits.I2C1TXIF = 0; // Clear the I2C1 transmit interrupt flag
        PIR7bits.I2C1RXIF = 0; // Clear the I2C1 receive interrupt flag
        I2C1PIR = 0;           // Clear I2C1 interrupt flags
        I2C1ERR = 0;           // Clear I2C1 error flags
        I2C1CNT = 0;           // Reset I2C1 count register
        I2C1CON0bits.EN = 1;   // Re-enable module so future transactions can run

        PIE7bits.I2C1EIE = 1; // Restore base interrupt gates; TX/RX are enabled per transfer.
        PIE7bits.I2C1IE = 1;
        PIE7bits.I2C1TXIE = 0;
        PIE7bits.I2C1RXIE = 0;

        handle->state = I2C_IDLE;    // Set the I2C bus state to idle
        handle->lastStatus = I2C_OK; // Set the last state to OK
        CRITICAL_SECTION_END();
        return I2C_OK;
    }
#endif

#ifdef I2C2CON0
    if (handle->channel == I2C_CHANNEL_2)
    {
        CRITICAL_SECTION_START();
        PIE6bits.I2C2TXIE = 0;
        PIE6bits.I2C2RXIE = 0;
        I2C2CON0bits.CSTR = 0;
        I2C2CON0bits.RSEN = 0;
        I2C2CON0bits.S = 0;
        I2C2CON0bits.EN = 0;
        PIR6bits.I2C2EIF = 0;
        PIR6bits.I2C2IF = 0;
        PIR6bits.I2C2TXIF = 0;
        PIR6bits.I2C2RXIF = 0;
        I2C2PIR = 0;
        I2C2ERR = 0;
        I2C2CNT = 0;
        I2C2CON0bits.EN = 1;

        PIE6bits.I2C2EIE = 1;
        PIE6bits.I2C2IE = 1;
        PIE6bits.I2C2TXIE = 0;
        PIE6bits.I2C2RXIE = 0;

        handle->state = I2C_IDLE;
        handle->lastStatus = I2C_OK;
        CRITICAL_SECTION_END();
        return I2C_OK;
    }
#endif

    return I2C_INVALID_CHANNEL;
}

/// @brief Check if the I2C bus is busy.  This function checks the state of the I2C bus and
/// returns an error code indicating whether the bus is busy or idle.  Thefunction checks
/// the i2c count and status register bits to determine if its idle or not.
I2C_Status_t I2C_IsBusy(I2C_Handle_t* handle)
{
    if (!isHandleValid(handle))
    {
        return I2C_INVALID_HANDLE; // Return an error code indicating that the handle is invalid
    }
    cacheHandle(handle);

#ifdef I2C1CON0
    if (handle->channel == I2C_CHANNEL_1)
    {
        if (I2C1STAT0bits.BFRE)
        {
            return I2C_OK; // Bus is free
        }
    }
#endif
#ifdef I2C2CON0
    if (handle->channel == I2C_CHANNEL_2)
    {
        if (I2C2STAT0bits.BFRE)
        {
            return I2C_OK; // Bus is free
        }
    }
#endif

    return I2C_BUSY; // Bus is busy
}

/// @brief Validate the I2C handle structure.  This function checks the signature of the
/// I2C handle structure to ensure that it is valid.  The function returns true if
/// the handle is valid, and false if the handle is invalid.  The function is defined as
/// follows:
static bool isHandleValid(I2C_Handle_t* handle)
{
    if (handle == NULL)
    {
        return false; // Handle is NULL, not valid
    }
    if (handle->signature != I2C_HANDLE_SIGNATURE)
    {
        return false; // Handle signature does not match, not valid
    }
    return true; // Handle is valid
}

/// @brief Setup the I2C device address for read or write operation.  This function sets
/// the I2C device address in the appropriate register based on the I2C channel and the
/// read/write operation.  The function is defined as follows:
static void setupDeviceAddress(I2C_Handle_t* handle, uint16_t deviceAddress, bool read)
{
    uint8_t addressHigh = (deviceAddress >> 8) & 0x03; // Get the upper 2 bits for 10-bit addressing
    addressHigh |= 0xF0;                       // Set the upper 4 bits to 1 for 10-bit addressing
    uint8_t addressLow = deviceAddress & 0xFF; // Get the lower 8 bits for 10-bit addressing
    if (read)
    {
        addressLow |= 0x01; // Set the read/write bit for read operation
    }
    else
    {
        addressLow &= ~0x01; // Clear the read/write bit for write operation
    }

#ifdef I2C1CON0
    if (handle->channel == I2C_CHANNEL_1)
    {
        switch (handle->mode)
        {
        case I2C_MODE_MASTER_7:
        case I2C_MULTI_MASTER_7:
            I2C1ADB1 = addressLow; // Set the 7-bit address in the ADB1 register
            break;
        case I2C_MODE_MASTER_10:
        case I2C_MULTI_MASTER_10:
            I2C1ADB1 = addressLow;  // Set the lower 8 bits for 10-bit addressing
            I2C1ADB0 = addressHigh; // Set the upper 2 bits for 10-bit addressing
            break;
        default:
            handle->lastStatus = I2C_INVALID_MODE; // Set the last state to invalid mode
            return;                                // Invalid mode, exit the function
        }
    }
#endif
#ifdef I2C2CON0
    if (handle->channel == I2C_CHANNEL_2)
    {
        switch (handle->mode)
        {
        case I2C_MODE_MASTER_7:
        case I2C_MULTI_MASTER_7:
            I2C2ADB1 = addressLow; // Set the 7-bit address in the ADB1 register
            break;
        case I2C_MODE_MASTER_10:
        case I2C_MULTI_MASTER_10:
            I2C2ADB1 = addressLow;  // Set the lower 8 bits for 10-bit addressing
            I2C2ADB0 = addressHigh; // Set the upper 2 bits for 10-bit addressing
            break;
        default:
            handle->lastStatus = I2C_INVALID_MODE; // Set the last state to invalid mode
            return;                                // Invalid mode, exit the function
        }
    }
#endif
}

/// @brief Handle I2C error interrupts.  This function checks the I2C error interrupt flags
/// and updates the I2C handle structure with the appropriate error state.
/// @param handle Pointer to the I2C handle structure.
/// @return None
static void handleErrorInterrupt(I2C_Handle_t* handle)
{
    if (!isHandleValid(handle))
    {
        return; // Handle is NULL, exit the function
    }

#ifdef I2C1CON0
    if (handle->channel == I2C_CHANNEL_1)
    {
        // Check for bus timeout error
        if (I2C1ERRbits.BTOIF)
        {
            I2C1ERRbits.BTOIF = 0;                // Clear the bus timeout interrupt flag
            handle->lastStatus = I2C_BUS_TIMEOUT; // Set the last state to bus timeout
            handle->state = I2C_IDLE;             // Set the I2C bus state to idle
        }

        // Check for bus collision error
        if (I2C1ERRbits.BCLIF)
        {
            I2C1ERRbits.BCLIF = 0;                  // Clear the bus collision interrupt flag
            handle->lastStatus = I2C_BUS_COLLISION; // Set the last state to bus collision
            handle->state = I2C_IDLE;               // Set the I2C bus state to idle
        }

        // Check for NACK received error
        if (I2C1ERRbits.NACKIF)
        {
            I2C1ERRbits.NACKIF = 0; // Clear the NACK interrupt flag
            handle->lastStatus =
                (I2C_Status_t)I2C_NACK_RECEIVED; // Set the last state to NACK received
            handle->state = I2C_IDLE;            // Set the I2C bus state to idle
        }
    }
#endif
}

/// @brief Handle I2C general interrupts. This is used to advance transactions
/// that complete in hardware without guaranteeing a follow-up TX pacing interrupt.
/// @param handle Pointer to the I2C handle structure.
/// @return None
static void handleGeneralInterrupt(I2C_Handle_t* handle)
{
    if (!isHandleValid(handle))
    {
        return; // Handle is NULL, exit the function
    }

#ifdef I2C1CON0
    if (handle->channel == I2C_CHANNEL_1)
    {
        // After the 1-byte register-address write phase, CNT reaches zero.
        // Trigger the repeated-start read phase here instead of depending on TXIF timing.
        if (handle->state == I2C_READ_REGISTER && I2C1CNT == 0)
        {
            setupDeviceAddress(handle, handle->activeDeviceAddress, true);
            handle->state = I2C_READ;
            PIE7bits.I2C1TXIE = 0;
            PIE7bits.I2C1RXIE = 1;
            I2C1CNT = (uint8_t)handle->rxBufferSize;
            I2C1CON0bits.RSEN = 1;
            I2C1CON0bits.CSTR = 0;
            I2C1CON0bits.S = 1;
        }
    }
#endif
}

/// @brief Handle I2C receive interrupts.  This function reads the received data from the
/// I2C receive buffer and stores it in the I2C handle structure.
/// @param handle Pointer to the I2C handle structure.
/// @return None
void handleReceiveInterrupt(I2C_Handle_t* handle)
{
    if (!isHandleValid(handle))
    {
        return; // Handle is NULL, exit the function
    }

#ifdef I2C1CON0
    if (handle->channel == I2C_CHANNEL_1)
    {
        uint8_t data = I2C1RXB; // Read the receive buffer to clear the interrupt flag
        if (handle->state != I2C_READ)
        {
            return; // Not in read mode, exit the ISR
        }

        if (handle->rxBufferIndex < handle->rxBufferSize)
        {
            handle->rxBuffer[handle->rxBufferIndex++] = data;
            if (handle->rxBufferIndex >= handle->rxBufferSize)
            {
                handle->state = I2C_IDLE;
                PIE7bits.I2C1RXIE = 0;
            }
        }
        else
        {
            handle->state = I2C_IDLE;
            PIE7bits.I2C1RXIE = 0;
        }
    }
#endif
}

/// @brief Handle I2C transmit interrupts.  This function writes the next byte of data to the
/// I2C transmit buffer and updates the I2C handle structure.  If all data has been transmitted,
/// the function disables the transmit interrupt and sets the I2C bus state to idle.  If the
/// function is in read register mode, it leaves the transition to read mode to the general
/// interrupt path after the write byte count reaches zero.
/// @param handle Pointer to the I2C handle structure.
/// @return None
void handleTransmitInterrupt(I2C_Handle_t* handle)
{
    if (!isHandleValid(handle))
    {
        return; // Handle is NULL, exit the function
    }

    if (handle->state != I2C_WRITE && handle->state != I2C_READ_REGISTER)
    {
        return; // Not in write mode, exit the ISR
    }

    if (handle->txBufferIndex < handle->txBufferSize)
    {
        // There is data to transmit
        I2C1TXB = handle->txBuffer[handle->txBufferIndex++];
        I2C1CON0bits.CSTR = 0;
    }
    else
    {
        // If TXB emptied before CNT reaches zero, the module may stretch SCL
        // waiting for software service. Explicitly release stretching so the
        // ACK/NACK clock can complete.
        I2C1CON0bits.CSTR = 0;

        if (handle->state == I2C_READ_REGISTER)
        {
            // Transition to read phase is handled by general interrupt on CNT completion.
            PIE7bits.I2C1TXIE = 0;
        }
        // Complete write-only transfer once hardware count is fully done.
        else if (I2C1CNT == 0)
        {
            handle->state = I2C_IDLE;
            PIE7bits.I2C1TXIE = 0; // Disable TX interrupt after final count completion
        }
    }
}

/// @brief Cache the I2C handle for the specified channel.
/// @param handle Pointer to the I2C handle structure to be cached.
static void cacheHandle(I2C_Handle_t* handle)
{
    if (!isHandleValid(handle))
    {
        return;
    }
    if (handle->channel == I2C_CHANNEL_1)
    {
        i2cHandles[0] = handle;
    }
    else if (handle->channel == I2C_CHANNEL_2)
    {
        i2cHandles[1] = handle;
    }
}