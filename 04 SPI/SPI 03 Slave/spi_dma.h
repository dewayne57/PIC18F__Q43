/* *****************************************************************************************
 *   File Name: spi_dma.h
 *   Description: DMA module API for SPI 02 DMA Burst Transfer.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#ifndef SPI_DMA_H
#define SPI_DMA_H

#include <stdbool.h>
#include <stdint.h>

void SPI_DMA_Initialize(void);
void SPI_DMA_Task(void);

#endif /* SPI_DMA_H */
