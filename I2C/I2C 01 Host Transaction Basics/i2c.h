/* *****************************************************************************************
 *   File Name: i2c.h
 *   Description: Public API for I2C 01 Host Transaction Basics.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#ifndef I2C_H
#define I2C_H

#include <stdbool.h>
#include <stdint.h>

void I2C1_Initialize(void);
void I2C1_Task(void);

#endif /* I2C_H */
