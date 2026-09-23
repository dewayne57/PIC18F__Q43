/* *****************************************************************************************
 *   File Name: pwm.h
 *   Description: PWM module configuration, control, and initialization for the demonstration
 *   project.
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

#ifndef PWM_H
#define PWM_H

#define PWM1_CLOCK_FREQ 64000000UL
#define PWM_SIGNATURE 0x01EF

/// @brief Enumeration for the status of PWM operations.
typedef enum
{
    PWM_SUCCESS = 0,
    PWM_ERROR = 1
} PWM_Status;

/// @brief Structure representing the handle for a PWM module.
typedef struct
{
    uint32_t ClockFrequency; // the clock frequency of the PWM module
    uint32_t Frequency;  // the desired PWM frequency
    uint16_t DutyPercent; // the desired PWM duty cycle percentage (0-100)
    uint16_t Period;     // the total period time of the PWM signal
    uint16_t DutyValue;  // the calculated duty period corresponding to the desired duty cycle
    uint16_t Signature;  // the signature to verify the handle's validity
    bool Initialized;    // indicates if the PWM module has been initialized
    bool Enabled;        // indicates if the PWM module is currently enabled
} PWM_Handle;

PWM_Status PWM_Open(PWM_Handle *handle, uint32_t frequency, uint16_t dutyPercent);
PWM_Status PWM_Close(PWM_Handle *handle);
PWM_Status PWM_SetDuty(PWM_Handle *handle, uint16_t dutyPercent);
PWM_Status PWM_Pause(PWM_Handle *handle);
PWM_Status PWM_Resume(PWM_Handle *handle);

#endif /* PWM_H */