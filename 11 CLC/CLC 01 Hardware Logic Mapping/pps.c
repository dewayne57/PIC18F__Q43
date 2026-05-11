/* *****************************************************************************************
 *   File Name: pps.c
 *   Description: Peripheral Pin Select (PPS) functions for PIC18F47Q43.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 * 
 *   Copyright (c) 2026, Dewayne Hafenstein.
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 * 
 *   This module provides functions to unlock and lock the PPS registers,
 *   which are necessary to configure the pin mappings for peripherals such as UART, SPI, 
 *   etc.  The PPS system allows for flexible assignment of peripheral functions to physical
 *   pins on the microcontroller.
 ***************************************************************************************** */

 #include "pps.h" 

/**
 * Functions to unlock and lock the PPS registers.  These functions are used to allow changes
 * to the PPS configuration, which is necessary to assign peripheral functions to specific pins. 
 */
void PPS_Unlock(void)
{
#if defined(PPSLOCK)
    PPSLOCK = 0x55U;
    PPSLOCK = 0xAAU;
    PPSLOCKbits.PPSLOCKED = 0U;
#endif
}

/**
 * Lock the PPS registers to prevent further changes.  This is typically done after configuring 
 * the PPS to ensure that the pin mappings are not accidentally modified during runtime.
 * Locking the PPS registers is a good practice to prevent unintended changes to the pin 
 * configuration, which could lead to unpredictable behavior or system errors.
 */
void PPS_Lock(void)
{
#if defined(PPSLOCK)
    PPSLOCK = 0x55U;
    PPSLOCK = 0xAAU;
    PPSLOCKbits.PPSLOCKED = 1U;
#endif
}
