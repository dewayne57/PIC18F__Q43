/* *****************************************************************************************
 *   File Name: dma.c
 *   Description: Shared DMA channel select and address/count helpers.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-01
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
 * ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include "config.h"
#include "dma.h"

/// @brief Return true while DMA channel DGO is asserted.
/// @note DGO (DMA Go) is the active-transfer flag.  It is set by software to start
///       a transfer and cleared automatically by hardware when the transfer finishes.
bool DMA_IsTransferInProgress(const uint8_t channel) 
{
    DMA_SelectChannel(channel);
    return (DMAnCON0bits.DGO != 0U);
}

/// @brief Selects the DMA channel to be used.
/// @param channel The DMA channel number (1-based).
/// @return None
void DMA_SelectChannel(const uint8_t channel)
{
    /* DMASELECT is a bank-select register that maps the DMAnCON0/CON1/SSA/DSA etc.
       registers to the selected channel.  The register is 0-based so channel 1 = DMASELECT=0,
       channel 2 = DMASELECT=1, and so on.  All DMAnXXX register accesses after this
       call target the selected channel until DMASELECT is changed again. */
    DMASELECT = (uint8_t)(channel - 1U);
}

/// @brief  Sets the source address for the specified DMA channel.
/// @param channel The DMA channel number (1-based).
/// @param data Pointer to the source data buffer.
/// @return None
void DMA_SetSourceAddress(const uint8_t channel, volatile uint8_t *data)
{
    uintptr_t addr = (uintptr_t)data;

    DMA_SelectChannel(channel);
    /* The PIC18 has a 21-bit program/data address space, so the full address needs
       three bytes (Low, High, Upper) to cover the entire range:
       SSAL = bits  7..0  (least significant byte)
       SSAH = bits 15..8  (middle byte)
       SSAU = bits 20..16 (upper 5 bits, only 3 used) */
    DMAnSSAL = (uint8_t)(addr & 0xFFU);          // Source address low byte
    DMAnSSAH = (uint8_t)((addr >> 8) & 0xFFU);   // Source address high byte
    DMAnSSAU = (uint8_t)((addr >> 16) & 0xFFU);  // Source address upper byte
}

/// @brief  Sets the destination address for the specified DMA channel.
/// @param channel The DMA channel number (1-based).
/// @param data Pointer to the destination data buffer.
/// @return None
void DMA_SetDestAddress(const uint8_t channel, volatile uint8_t *data)
{
    uintptr_t addr = (uintptr_t)data;

    DMA_SelectChannel(channel);
    /* The destination is always a Special Function Register (SFR) which lives in
       the lower 12-bit SFR address space, so only two bytes are needed here
       (no upper byte for destination addresses). */
    DMAnDSAL = (uint8_t)(addr & 0xFFU);         // Destination address low byte
    DMAnDSAH = (uint8_t)((addr >> 8) & 0xFFU);  // Destination address high byte
}

/// @brief  Sets the transfer count for the specified DMA channel.
/// @param channel The DMA channel number (1-based).
/// @param count The number of bytes to transfer.
/// @return None
void DMA_SetTransferCount(const uint8_t channel, const uint16_t count)
{
    DMA_SelectChannel(channel);
    /* Source size (SSIZ) is the number of bytes to copy from the source buffer.
       Each DMA trigger copies one byte and decrements SSCNT.  When SSCNT reaches
       zero the SCNT interrupt fires and (with SSTP=1) the DMA stops automatically. */
    DMAnSSZL = (uint8_t)(count & 0xFFU);        // Source byte count low
    DMAnSSZH = (uint8_t)((count >> 8) & 0xFFU); // Source byte count high
    /* Destination size is fixed at 1 because the destination is a single hardware
       register (U1TXB).  The destination pointer never advances (DMODE=0), so
       the destination count does not limit the transfer; only the source count does. */
    DMAnDSZL = 1U;   // Destination count = 1 (single fixed register target)
    DMAnDSZH = 0U;
}
