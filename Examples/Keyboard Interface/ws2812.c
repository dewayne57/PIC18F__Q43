/* *****************************************************************************************
 *   File Name: ws2812.c
 *   Description: WS2812 NeoPixel LED strip interface for the demonstration project.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-19
 *
 *   Configure Port B/C pins for WS2812 data as a GPIO output.
 *
 ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "../../Libraries/INTLIB/intlib.h"
#include "config.h"
#include "ws2812.h"
#include "ditto.h"
#include "../../Libraries/PPSLIB/pps.h"

#define WS2812_BIT_RATE_HZ 800000UL
#define WS2812_T0H_NS 400UL
#define WS2812_T1H_NS 800UL
#define WS2812_BITS_PER_LED 24U

/// @brief Configure the specified data pin for use with the WS2812 strip.
/// @param dataPin Data pin to configure.
/// @return WS2812_OK if the operation was successful, otherwise an error code.
static WS2812_Status_t setupDataPin(WS2812_DATA_PIN dataPin) {
    volatile uint8_t *latRegister = (volatile uint8_t *) 0;
    uint8_t bitMask = 0U;

    switch (dataPin) {
        case WS2812_PIN_RB0:
            ANSELBbits.ANSELB0 = 0U;
            TRISBbits.TRISB0 = 0U;
            RB0PPS = 0x00U;
            latRegister = &LATB;
            bitMask = 0x01U;
            break;
        case WS2812_PIN_RB1:
            ANSELBbits.ANSELB1 = 0U;
            TRISBbits.TRISB1 = 0U;
            RB1PPS = 0x00U;
            latRegister = &LATB;
            bitMask = 0x02U;
            break;
        case WS2812_PIN_RB2:
            ANSELBbits.ANSELB2 = 0U;
            TRISBbits.TRISB2 = 0U;
            RB2PPS = 0x00U;
            latRegister = &LATB;
            bitMask = 0x04U;
            break;
        case WS2812_PIN_RB3:
            ANSELBbits.ANSELB3 = 0U;
            TRISBbits.TRISB3 = 0U;
            RB3PPS = 0x00U;
            latRegister = &LATB;
            bitMask = 0x08U;
            break;
        case WS2812_PIN_RB4:
            ANSELBbits.ANSELB4 = 0U;
            TRISBbits.TRISB4 = 0U;
            RB4PPS = 0x00U;
            latRegister = &LATB;
            bitMask = 0x10U;
            break;
        case WS2812_PIN_RB5:
            ANSELBbits.ANSELB5 = 0U;
            TRISBbits.TRISB5 = 0U;
            RB5PPS = 0x00U;
            latRegister = &LATB;
            bitMask = 0x20U;
            break;
        case WS2812_PIN_RB6:
            ANSELBbits.ANSELB6 = 0U;
            TRISBbits.TRISB6 = 0U;
            RB6PPS = 0x00U;
            latRegister = &LATB;
            bitMask = 0x40U;
            break;
        case WS2812_PIN_RB7:
            ANSELBbits.ANSELB7 = 0U;
            TRISBbits.TRISB7 = 0U;
            RB7PPS = 0x00U;
            latRegister = &LATB;
            bitMask = 0x80U;
            break;
        case WS2812_PIN_RC0:
            ANSELCbits.ANSELC0 = 0U;
            TRISCbits.TRISC0 = 0U;
            RC0PPS = 0x00U;
            latRegister = &LATC;
            bitMask = 0x01U;
            break;
        case WS2812_PIN_RC1:
            ANSELCbits.ANSELC1 = 0U;
            TRISCbits.TRISC1 = 0U;
            RC1PPS = 0x00U;
            latRegister = &LATC;
            bitMask = 0x02U;
            break;
        case WS2812_PIN_RC2:
            ANSELCbits.ANSELC2 = 0U;
            TRISCbits.TRISC2 = 0U;
            RC2PPS = 0x00U;
            latRegister = &LATC;
            bitMask = 0x04U;
            break;
        case WS2812_PIN_RC3:
            ANSELCbits.ANSELC3 = 0U;
            TRISCbits.TRISC3 = 0U;
            RC3PPS = 0x00U;
            latRegister = &LATC;
            bitMask = 0x08U;
            break;
        case WS2812_PIN_RC4:
            ANSELCbits.ANSELC4 = 0U;
            TRISCbits.TRISC4 = 0U;
            RC4PPS = 0x00U;
            latRegister = &LATC;
            bitMask = 0x10U;
            break;
        case WS2812_PIN_RC5:
            ANSELCbits.ANSELC5 = 0U;
            TRISCbits.TRISC5 = 0U;
            RC5PPS = 0x00U;
            latRegister = &LATC;
            bitMask = 0x20U;
            break;
        case WS2812_PIN_RC6:
            ANSELCbits.ANSELC6 = 0U;
            TRISCbits.TRISC6 = 0U;
            RC6PPS = 0x00U;
            latRegister = &LATC;
            bitMask = 0x40U;
            break;
        case WS2812_PIN_RC7:
            ANSELCbits.ANSELC7 = 0U;
            TRISCbits.TRISC7 = 0U;
            RC7PPS = 0x00U;
            latRegister = &LATC;
            bitMask = 0x80U;
            break;
        default:
            return WS2812_INVALID_PARAM;
    }

    *latRegister &= (uint8_t) ~bitMask;

    return WS2812_OK;
}

/// @brief Initialize the WS2812 strip with the specified data pin and number of LEDs.
/// @param strip Pointer to the WS2812 strip to initialize.
/// @param dataPin Data pin to use for the WS2812 strip.
/// @param numLEDs Number of LEDs in the WS2812 strip.
/// @return WS2812_OK if the operation was successful, otherwise an error code.
WS2812_Status_t WS2812_Init(WS2812_Strip_t *strip, 
        WS2812_DATA_PIN dataPin, uint16_t numLEDs) {
    WS2812_Status_t status;

    if ((strip == NULL) || (numLEDs == 0U) || (numLEDs > WS2812_MAX_LEDS)) {
        return WS2812_INVALID_PARAM;
    }

    if (strip->initialized) {
        return WS2812_ERROR;
    }

    strip->signature = WS2812_SIGNATURE;
    strip->initialized = true;
    strip->busy = false;
    strip->numLEDs = numLEDs;
    strip->dataPin = dataPin;

    INTCON0bits.GIEH = 0U;
    INTCON0bits.GIEL = 0U;
    PPS_Unlock();
    status = setupDataPin(dataPin);
    PPS_Lock();
    INTCON0bits.GIEH = 1U;
    INTCON0bits.GIEL = 1U;

    return status;
}

/// @brief Set the color of a specific LED in the WS2812 strip.
/// @param strip Pointer to the WS2812 strip.
/// @param index Index of the LED to set.
/// @param color Color to set the LED to.
/// @return WS2812_OK if the operation was successful, otherwise an error code.
WS2812_Status_t WS2812_SetColor(WS2812_Strip_t *strip, uint16_t index, WS2812_Color_t color) {
    if (strip == NULL || !strip->initialized) {
        return WS2812_NOT_INITIALIZED;
    }

    if (index >= strip->numLEDs) {
        return WS2812_OUT_OF_BOUNDS;
    }

    strip->colors[index] = color;
    return WS2812_OK;
}

/// @brief Update the WS2812 strip with the current color values. 
/// @param strip Pointer to the WS2812 strip to update.
/// @return WS2812_OK if the operation was successful, otherwise an error code.
WS2812_Status_t WS2812_Update(WS2812_Strip_t *strip) {
    if (strip == NULL || !strip->initialized) {
        return WS2812_NOT_INITIALIZED;
    }

    if (strip->busy) {
        return WS2812_BUSY;
    }

    CRITICAL_SECTION_START();
    strip->busy = true;

    for (int i = 0; i < strip->numLEDs; i++) {
        // Send the color data for each LED to the WS2812 strip.
        // This typically involves sending the green, red, and blue components in that order.
        // Each component is 8 bits, and each bit is sent with precise timing according to the WS2812 protocol.
        for (int j = 0; j < 3; j++) {
            uint8_t component = ((uint8_t *)&strip->colors[i])[j];
            for (int k = 7; k >= 0; k--) {
                // Send each bit of the component to the WS2812 strip with precise timing.
                // This is typically done using assembly language for maximum speed.
                if (component & (1 << k)) {
                    asm("BSF   LATB, 5\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "BCF   LATB, 5\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "nop\n\t"
                    );
                } else {
                    asm (
                        "BSF LATB, 5\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "BCF LATB, 5\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "nop\n\t"
                        "nop\n\t"
                    );
                }
            }
        }
    }

    CRITICAL_SECTION_END();
    strip->busy = false;

    return WS2812_OK;
}

/// @brief Clear  all LEDs to GRB(0,0,0). 
/// @param strip Pointer to the WS2812 strip to clear.
/// @return WS2812_OK if the operation was successful, otherwise an error code.
WS2812_Status_t WS2812_Clear(WS2812_Strip_t *strip) {
    if (strip == NULL || !strip->initialized) {
        return WS2812_NOT_INITIALIZED;
    }

    for (uint16_t i = 0U; i < strip->numLEDs; i++) {
        strip->colors[i] = (WS2812_Color_t){0U, 0U, 0U};
    }

    return WS2812_Update(strip);
}

