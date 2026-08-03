/* *****************************************************************************************
 *   File Name: dmalib.h
 *   Description: Shared DMA channel and address helper functions for PIC18F Q43 projects.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-08-03
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

#ifndef DMALIB_H
#define DMALIB_H

#include <stdbool.h>
#include <stdint.h>

bool DMA_IsTransferInProgress(const uint8_t channel);
void DMA_SelectChannel(const uint8_t channel);
void DMA_SetSourceAddress(const uint8_t channel, volatile uint8_t *data);
void DMA_SetDestAddress(const uint8_t channel, volatile uint8_t *data);
void DMA_SetTransferCount(const uint8_t channel, const uint16_t count);

#endif /* DMALIB_H */
