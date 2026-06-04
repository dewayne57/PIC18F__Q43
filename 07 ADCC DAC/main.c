/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Continuous ADCC window comparator demo (LOW/HIGH/IN-WINDOW LEDs).
 *   Author: Dewayne Hafenstein
 *   Date: 2026-06-04
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"

#define LED_LOW_ON() (LATBbits.LATB2 = 0)
#define LED_LOW_OFF() (LATBbits.LATB2 = 1)
#define LED_HIGH_ON() (LATBbits.LATB3 = 0)
#define LED_HIGH_OFF() (LATBbits.LATB3 = 1)
#define LED_WINDOW_ON() (LATBbits.LATB4 = 0)
#define LED_WINDOW_OFF() (LATBbits.LATB4 = 1)

static void LED_ShowLow(void)
{
    LED_LOW_ON();
    LED_HIGH_OFF();
    LED_WINDOW_OFF();
}

static void LED_ShowHigh(void)
{
    LED_LOW_OFF();
    LED_HIGH_ON();
    LED_WINDOW_OFF();
}

static void LED_ShowInWindow(void)
{
    LED_LOW_OFF();
    LED_HIGH_OFF();
    LED_WINDOW_ON();
}

void __interrupt(irq(IRQ_AD), high_priority) ADC_ISR(void)
{
    if (PIR1bits.ADIF)
    {
        uint16_t adc_result = ADRES;
        uint16_t low_threshold = ADLTH;
        uint16_t high_threshold = ADUTH;

        PIR1bits.ADIF = 0;

        if (adc_result < low_threshold)
        {
            LED_ShowLow();
        }
        else if (adc_result > high_threshold)
        {
            LED_ShowHigh();
        }
        else
        {
            LED_ShowInWindow();
        }
    }
}

void main(void)
{
    SYSTEM_Initialize();

    while (1)
    {
        /* ADCC runs continuously and LED state is updated in ADC_ISR. */
    }
}
