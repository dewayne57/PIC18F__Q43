/* *****************************************************************************************
 *   File Name: pwm.h
 *   Description: Public API for PWM 01 Edge Aligned Duty Control.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#ifndef PWM_H
#define PWM_H

#include <stdbool.h>
#include <stdint.h>

void PWM1_Initialize(void);
void PWM1_Task(void);

#endif /* PWM_H */
