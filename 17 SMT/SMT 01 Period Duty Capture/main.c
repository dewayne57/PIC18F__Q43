/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for SMT 01 Period Duty Capture.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 *
 *   Copyright (c) 2026, Dewayne Hafenstein.
 *   Licensed under the Apache License, Version 2.0.
 ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "smt.h"

static void APP_UpdateIndicators(const smt_capture_t *capture)
{
    /* 0-1000 permil mapped to 0-8 LEDs on PORTD. */
    uint8_t on_count = (uint8_t)((capture->duty_permil + 62U) / 125U);
    uint8_t mask = (on_count >= 8U) ? 0xFFU : (uint8_t)((1U << on_count) - 1U);
    LATD = mask;
}

void main(void)
{
    smt_capture_t capture;

    SYSTEM_Initialize();
    SMT1_Initialize();

    while (1)
    {
        SMT1_CaptureTask();

        if (SMT1_GetLatestCapture(&capture))
        {
            APP_UpdateIndicators(&capture);
        }
    }
}
