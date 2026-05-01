/* *****************************************************************************************
 *   File Name: adcc_dma.h
 *   Description: DMA module API for ADCC 03 DMA Sample Buffer.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#ifndef ADCC_DMA_H
#define ADCC_DMA_H

#include <stdbool.h>
#include <stdint.h>

void ADCC_DMA_Initialize(void);
void ADCC_DMA_Task(void);

#endif /* ADCC_DMA_H */
