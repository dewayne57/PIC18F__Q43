/* *****************************************************************************************
 *   File Name: dma.h
 *   Description: Shared DMA channel select and address/count helper declarations.
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

#ifndef DMA_H
#define DMA_H
#include <stdbool.h>
#include <stdint.h>

/// @brief Check whether a DMA channel currently has an active transfer.
/// @param channel DMA channel number.
/// @return true if transfer is in progress, false otherwise.
bool DMA_IsTransferInProgress(const uint8_t channel);

/// @brief Select a DMA channel as the active channel for DMAn register access.
/// @param channel DMA channel number.
/// @return None
void DMA_SelectChannel(const uint8_t channel);

/// @brief Set DMA source start address for the selected channel.
/// @param channel DMA channel number.
/// @param data Pointer to source data.
/// @return None
void DMA_SetSourceAddress(const uint8_t channel, volatile uint8_t *data);

/// @brief Set DMA destination start address for the selected channel.
/// @param channel DMA channel number.
/// @param data Pointer to destination register or memory.
/// @return None
void DMA_SetDestAddress(const uint8_t channel, volatile uint8_t *data);

/// @brief Set DMA transfer element count for the selected channel.
/// @param channel DMA channel number.
/// @param count Number of elements to transfer.
/// @return None
void DMA_SetTransferCount(const uint8_t channel, const uint16_t count);
#endif /* DMA_H */
