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

/// @brief Turns on the "LOW" LED and turns off the other LEDs.
/// @param  None
/// @return None
static void LED_ShowLow(void)
{
    LED_LOW_ON();
    LED_HIGH_OFF();
    LED_WINDOW_OFF();
}

/// @brief Turns on the "HIGH" LED and turns off the other LEDs.
/// @param  None
/// @return None
static void LED_ShowHigh(void)
{
    LED_LOW_OFF();
    LED_HIGH_ON();
    LED_WINDOW_OFF();
}

/// @brief  Turns on the "IN-WINDOW" LED and turns off the other LEDs.
/// @param  None
/// @return None
static void LED_ShowInWindow(void)
{
    LED_LOW_OFF();
    LED_HIGH_OFF();
    LED_WINDOW_ON();
}

/// @brief ADC interrupt service routine. Determines if the ADC result is below, above, or 
/// within the defined thresholds and updates LEDs accordingly.  
/// @param  None
/// @return None
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

/// @brief Main function. Initializes the system and enters an infinite loop.
/// The ADCC runs continuously, and the LED states are updated in the ADC_ISR 
/// based on the ADC results.
/// @param  None
/// @return None    
void main(void)
{
    SYSTEM_Initialize();

    while (1)
    {
        /* ADCC runs continuously and LED state is updated in ADC_ISR. */
    }
}
