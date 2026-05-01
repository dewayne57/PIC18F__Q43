/* *****************************************************************************************
 *   File Name: timer.h
 *   Description: Public API for TMR 01 Periodic Interrupt Tick.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>
#include <stdint.h>

void TMR0_Initialize(void);
void TMR0_Task(void);

#endif /* TIMER_H */
