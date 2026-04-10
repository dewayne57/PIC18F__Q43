/* *****************************************************************************************
 *   File Name: smt.h
 *   Description: Capture interface for SMT 01 project.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#ifndef SMT_H
#define SMT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint32_t period_ticks;
    uint32_t high_ticks;
    uint16_t duty_permil; /* 0..1000 */
} smt_capture_t;

void SMT1_Initialize(void);
void SMT1_CaptureTask(void);
bool SMT1_GetLatestCapture(smt_capture_t *capture);

#endif /* SMT_H */
