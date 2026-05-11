/* *****************************************************************************************
 *   File Name: extern_ioc.h
 *   Description: External interrupt on change (IOC) handler for RB2.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-11
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
 *   RB2 - External interrupt input with IOC enabled on both rising and falling edges
 ***************************************************************************************** */

#ifndef EXTERN_IOC_H
#define EXTERN_IOC_H

#include <stdbool.h>
#include <stdint.h>

/// @brief Initialize external interrupt on change (IOC) for RB2
/// @param None
/// @return None
/// @note RB2 is configured as a digital input with weak pull-ups enabled
/// @note IOC is enabled on both rising and falling edges
void ExternIoc_Initialize(void);

/// @brief External interrupt on change handler for RB2
/// @param None
/// @return None
/// @note This function should be called from the ISR when an IOC event occurs on RB2
/// @note The function reads the current state of RB2 and can be used for any application-specific handling
void ExternIoc_HandleInterrupt(void);

#endif // EXTERN_IOC_H
