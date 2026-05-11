/* *****************************************************************************************
 *   File Name: zcd.h
 *   Description: Public API for ZCD 01 Zero Crossing Timestamp.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#ifndef ZCD_H
#define ZCD_H

#include <stdbool.h>
#include <stdint.h>

void ZCD1_Initialize(void);
void ZCD1_Task(void);

#endif /* ZCD_H */
