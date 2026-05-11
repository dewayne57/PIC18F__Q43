/* *****************************************************************************************
 *   File Name: smt.c
 *   Description: Minimal capture engine scaffold for SMT 01.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "smt.h"

static uint32_t s_ticks;
static uint32_t s_last_rise;
static uint32_t s_last_fall;
static uint8_t s_prev_level;
static bool s_has_new_capture;
static smt_capture_t s_capture;

void SMT1_Initialize(void)
{
    s_ticks = 0U;
    s_last_rise = 0U;
    s_last_fall = 0U;
    s_prev_level = ((PORTB & APP_SMT_INPUT_MASK) != 0U) ? 1U : 0U;
    s_has_new_capture = false;

    s_capture.period_ticks = 0U;
    s_capture.high_ticks = 0U;
    s_capture.duty_permil = 0U;

    /* TODO: Replace the software edge tracker with hardware SMT capture mode setup. */
}

void SMT1_CaptureTask(void)
{
    uint8_t level;

    s_ticks++;
    level = ((PORTB & APP_SMT_INPUT_MASK) != 0U) ? 1U : 0U;

    if ((level != 0U) && (s_prev_level == 0U))
    {
        uint32_t this_rise = s_ticks;

        if (s_last_rise != 0U)
        {
            s_capture.period_ticks = this_rise - s_last_rise;
            s_capture.high_ticks = s_last_fall - s_last_rise;

            if ((s_capture.period_ticks != 0U) && (s_capture.high_ticks <= s_capture.period_ticks))
            {
                s_capture.duty_permil = (uint16_t)((1000UL * s_capture.high_ticks) / s_capture.period_ticks);
                s_has_new_capture = true;
            }
        }

        s_last_rise = this_rise;
    }
    else if ((level == 0U) && (s_prev_level != 0U))
    {
        s_last_fall = s_ticks;
    }

    s_prev_level = level;
}

bool SMT1_GetLatestCapture(smt_capture_t *capture)
{
    if ((!s_has_new_capture) || (capture == 0))
    {
        return false;
    }

    *capture = s_capture;
    s_has_new_capture = false;
    return true;
}
