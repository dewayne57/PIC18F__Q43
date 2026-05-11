/* *****************************************************************************************
 *   File Name: spi.h
 *   Description: Public API for SPI 01 Host Loopback Transfer.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#ifndef SPI_H
#define SPI_H

#include <stdbool.h>
#include <stdint.h>

void SPI1_Initialize(void);
void SPI1_Task(void);

#endif /* SPI_H */
