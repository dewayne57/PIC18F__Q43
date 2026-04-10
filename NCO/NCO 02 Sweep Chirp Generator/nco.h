/* *****************************************************************************************
 *   File Name: nco.h
 *   Description: NCO interface for NCO 02 project.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#ifndef NCO_H
#define NCO_H

#include <stdint.h>

void NCO1_Initialize(void);
void NCO1_SetIncrement(uint32_t increment);
uint32_t NCO1_GetIncrement(void);
void NCO1_Task(void);

#endif /* NCO_H */
