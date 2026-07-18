/* *****************************************************************************************
 *   File Name: app.c
 *   Description: Hardware I2C1 module master implementation for PIC18F__Q43.
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
 * ***************************************************************************************** */

#ifndef INTLIB_H
#define INTLIB_H
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef CRITICAL_SECTION_START
#define CRITICAL_SECTION_START() \
    do { \
    uint8_t saved_intmask_h = INTCON0bits.GIEH; \
    INTCON0bits.GIEH = 0; \
    uint8_t saved_intmask_l = INTCON0bits.GIEL; \
    INTCON0bits.GIEL = 0;
#endif

#ifndef CRITICAL_SECTION_END
#define CRITICAL_SECTION_END() \
    INTCON0bits.GIEH = saved_intmask_h; \
    INTCON0bits.GIEL = saved_intmask_l; \
    } while (0);
#endif

#endif /* INTLIB_H */
