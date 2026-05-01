/* *****************************************************************************************
 *   File Name: cmp.h
 *   Description: Public API for CMP 01 Threshold Interrupt Event.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#ifndef CMP_H
#define CMP_H

#include <stdbool.h>
#include <stdint.h>

void CMP1_Initialize(void);
void CMP1_Task(void);

#endif /* CMP_H */
