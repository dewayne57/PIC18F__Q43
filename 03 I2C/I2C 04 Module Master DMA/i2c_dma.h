/* *****************************************************************************************
 *   File Name: i2c_dma.h
 *   Description: DMA module API for I2C 04 Module Master DMA.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#ifndef I2C_DMA_H
#define I2C_DMA_H

#include <stdbool.h>
#include <stdint.h>

void I2C_DMA_Initialize(void);
void I2C_DMA_Task(void);

#endif /* I2C_DMA_H */
