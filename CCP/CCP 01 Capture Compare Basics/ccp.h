/* *****************************************************************************************
 *   File Name: ccp.h
 *   Description: Public API for CCP 01 Capture Compare Basics.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#ifndef CCP_H
#define CCP_H

#include <stdbool.h>
#include <stdint.h>

void CCP1_Initialize(void);
void CCP1_Task(void);

#endif /* CCP_H */
