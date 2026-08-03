/* *****************************************************************************************
 *   File Name: dmalib.c
 *   Description: Shared DMA channel select and address/count helpers.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-08-03
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
#include "dmalib.h"

/// @brief Return true while DMA channel DGO is asserted.
/// @note DGO (DMA Go) is the active-transfer flag. It is set by software to start
///       a transfer and cleared automatically by hardware when the transfer finishes.
bool DMA_IsTransferInProgress(const uint8_t channel)
{
    DMA_SelectChannel(channel);
    // Some peripherals leave DGO asserted after the final trigger edge; SCNT is
    // the reliable indication that source bytes are still pending.
    return ((DMAnCON0bits.DGO != 0U) && (DMAnSCNT != 0U));
}

/// @brief Selects the DMA channel to be used.
/// @param channel The DMA channel number (1-based).
/// @return None
void DMA_SelectChannel(const uint8_t channel)
{
    DMASELECT = (uint8_t)(channel - 1U);
}

/// @brief Sets the source address for the specified DMA channel.
/// @param channel The DMA channel number (1-based).
/// @param data Pointer to the source data buffer.
/// @return None
void DMA_SetSourceAddress(const uint8_t channel, volatile uint8_t *data)
{
    uintptr_t addr = (uintptr_t)data;

    DMA_SelectChannel(channel);
    DMAnSSAL = (uint8_t)(addr & 0xFFU);
    DMAnSSAH = (uint8_t)((addr >> 8) & 0xFFU);
    DMAnSSAU = (uint8_t)((addr >> 16) & 0xFFU);
}

/// @brief Sets the destination address for the specified DMA channel.
/// @param channel The DMA channel number (1-based).
/// @param data Pointer to the destination data buffer.
/// @return None
void DMA_SetDestAddress(const uint8_t channel, volatile uint8_t *data)
{
    uintptr_t addr = (uintptr_t)data;

    DMA_SelectChannel(channel);
    DMAnDSAL = (uint8_t)(addr & 0xFFU);
    DMAnDSAH = (uint8_t)((addr >> 8) & 0xFFU);
}

/// @brief Sets the transfer count for the specified DMA channel.
/// @param channel The DMA channel number (1-based).
/// @param count The number of bytes to transfer.
/// @return None
void DMA_SetTransferCount(const uint8_t channel, const uint16_t count)
{
    DMA_SelectChannel(channel);
    DMAnSSZL = (uint8_t)(count & 0xFFU);
    DMAnSSZH = (uint8_t)((count >> 8) & 0xFFU);
    DMAnDSZL = 1U;
    DMAnDSZH = 0U;
}
