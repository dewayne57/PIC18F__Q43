/* *****************************************************************************************
 *   File Name: pwm.c
 *   Description: Configuration and initialization for the demonstration project.
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
#include "config.h"
#include "pwm.h"
#include <stdbool.h>

/// @brief Wait for any pending PWM load operation to complete.
/// @param None.
static void WaitForPendingLoad(void)
{
    /*
     * Wait if a previous PWM load operation is still pending.
     */
    while (PWM1CONbits.LD)
    {
        ;
    }
}

/// @brief  When the PWM handle was opened, the only parameters provided were the frequency
/// and the desired duty cycle, ranging from 0-100.  Additionally, once the PWM channel has
/// been opened, the duty cycle can be changed to any value between 0-100.  So, we need to
/// calculate the corresponding duty period based on the PWM period time and desired duty
/// cycle percentage.
/// @param handle Pointer to the PWM handle structure.
/// @param dutyPercent The desired PWM duty cycle percentage (0-100).
/// @return The calculated duty period corresponding to the given duty cycle percentage.
/// Returns the total PWM period time (100%) if the duty cycle is invalid.
static long CalculateDutyPeriod(PWM_Handle *handle, uint16_t dutyPercent)
{
    if (dutyPercent < 0 || dutyPercent > 100)
    {
        return handle->Period; // Invalid duty cycle, return total period time (100%)
    }

    return (uint16_t)((((uint32_t)handle->Period *
                        (uint32_t)dutyPercent) +
                       50UL) /
                      100UL);
}

/// @brief Initialize the PWM output for fan control and sets the initial PWM freuency and
/// duty cycle.
/// @param  handle Pointer to the PWM handle structure.
/// @param  frequency The desired PWM frequency.
/// @param  dutyPercent The desired PWM duty cycle percentage.
/// @return True if initialization is successful, false otherwise.  If called to initialize
/// the PWM module that is already initialized, it will return success without reinitializing.
/// You must call the PWM_Close function to properly deinitialize the PWM module.
PWM_Status PWM_Open(PWM_Handle *handle, uint32_t frequency, uint16_t dutyPercent)
{
    // No handle to initialize
    if (handle == NULL)
    {
        return PWM_ERROR;
    }

    // Handle is already initialized
    if (handle->Signature == PWM_SIGNATURE && handle->Initialized)
    {
        return PWM_SUCCESS;
    }

    PWM1CONbits.EN = 0; // Disable the PWM module
    handle->Signature = PWM_SIGNATURE;
    handle->Initialized = false;
    handle->Enabled = false;
    handle->ClockFrequency = PWM1_CLOCK_FREQ;
    handle->Frequency = frequency;
    handle->DutyPercent = dutyPercent;
    // Calculate the period corresponding to the desired frequency assuming a duty cycle of
    // 100% initially.  This is the total period time of the PWM signal.  The duty cycle
    // can then be adjusted at any time.
    handle->Period = (long)(handle->ClockFrequency / handle->Frequency);

    // Calculate the initial duty value based on the desired duty cycle percentage
    handle->DutyValue = CalculateDutyPeriod(handle, dutyPercent);

    PWM1ERS = 0x00;                  // Disable external reset source
    PWM1CLK = 0x03;                  // Set the PWM clock source to HFINTOSC
    PWM1LDS = 0x00;                  // Disable the PWM load source
    PWM1PRH = handle->Period >> 8;   // Set the PWM period high byte to the upper 8 bits of the period
    PWM1PRL = handle->Period & 0xFF; // Set the PWM period low byte to the lower 8 bits of the period
    PWM1CPRE = 0x00;                 // Set the PWM clock prescaler to 1:1
    PWM1PIPOS = 0x00;                // Set the PWM Period Interrupt Postscaler Register to 1:1
    PWM1GIE = 0x00;                  // Disable the PWM global interrupt
    PWM1CON = 0x00;                  // Reset all PWM control bits
    PWM1SACFGbits.POL2 = 0;          // Paramter 2 polarity is active high
    PWM1SACFGbits.POL1 = 0;          // Paramter 1 polarity is active high
    PWM1SACFGbits.PPEN = 0;          // Push-pull mode disabled
    PWM1SACFGbits.MODE = 0x00;       // Left aligned mode
    PWM1SAP1H = 0x00;                // Clear Slice A parameter 1 high register
    PWM1SAP1L = 0x00;                // Clear Slice A parameter 1 low register
    PWM1SAP2H = 0x00;                // Clear Slice A parameter 2 high register
    PWM1SAP2L = 0x00;                // Clear Slice A parameter 2 low register
    PWM1CONbits.EN = 1;              // Enable the PWM module
    handle->Initialized = true;      // Mark the handle as initialized
    handle->Enabled = true;          // Mark the handle as enabled

    return PWM_SetDuty(handle, dutyPercent);
}

/// @brief Close the PWM module and deinitialize the handle.
/// @param handle Pointer to the PWM handle to be closed.
/// @return PWM_SUCCESS if the module was successfully closed, PWM_ERROR otherwise.
PWM_Status PWM_Close(PWM_Handle *handle)
{
    if (handle == NULL)
    {
        return PWM_ERROR;
    }

    if (handle->Signature != PWM_SIGNATURE || !handle->Initialized)
    {
        return PWM_ERROR;
    }

    PWM1CONbits.EN = 0; // Disable the PWM module
    handle->Initialized = false;
    handle->Enabled = false;
    handle->Signature = 0;

    return PWM_SUCCESS;
}

/// @brief Set the duty cycle of the PWM output.
/// @param handle Pointer to the PWM handle.
/// @param dutyPercent Duty cycle percentage (0-100).  Note, if the value is outside this range,
/// an error response is generated and the current duty cycle remains unchanged.
/// @return PWM_SUCCESS if the duty cycle was successfully updated, PWM_ERROR otherwise.
PWM_Status PWM_SetDuty(PWM_Handle *handle, int dutyPercent)
{
    if (handle == NULL)
    {
        return PWM_ERROR;
    }

    if (handle->Signature != PWM_SIGNATURE || !handle->Initialized)
    {
        return PWM_ERROR;
    }

    if (dutyPercent < 0 || dutyPercent > 100)
    {
        return PWM_ERROR;
    }

    handle->DutyPercent = dutyPercent;
    handle->DutyValue = CalculateDutyPeriod(handle->Period, dutyPercent);

    WaitForPendingLoad();

    // Update the PWM hardware registers accordingly
    PWM1SAP1H = (handle->DutyValue >> 8) & 0xFF;
    PWM1SAP1L = handle->DutyValue & 0xFF;

    // Set the load bit to cause the new duty cycle to take effect
    PWM1CONbits.LD = 1;

    return PWM_SUCCESS;
}

/// @brief Pause the PWM module, disabling its output temporarily.
/// @param handle Pointer to the PWM handle.
/// @return PWM_SUCCESS if the module was successfully paused, PWM_ERROR otherwise.
PWM_Status PWM_Pause(PWM_Handle *handle)
{
    if (handle == NULL)
    {
        return PWM_ERROR;
    }

    if (handle->Signature != PWM_SIGNATURE || !handle->Initialized)
    {
        return PWM_ERROR;
    }

    PWM1CONbits.EN = 0; // Disable the PWM module
    handle->Enabled = false;

    return PWM_SUCCESS;
}

/// @brief Resume the PWM module, re-enabling its output.
/// @param handle Pointer to the PWM handle.
/// @return PWM_SUCCESS if the module was successfully resumed, PWM_ERROR otherwise.
PWM_Status PWM_Resume(PWM_Handle *handle)
{
    if (handle == NULL)
    {
        return PWM_ERROR;
    }

    if (handle->Signature != PWM_SIGNATURE || !handle->Initialized)
    {
        return PWM_ERROR;
    }

    PWM1CONbits.EN = 1; // Enable the PWM module
    handle->Enabled = true;

    return PWM_SUCCESS;
}