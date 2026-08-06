/* *****************************************************************************************
 *   File Name: ws28112.c
 *   Description: WS2812 NeoPixel LED strip interface for the demonstration project.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-19
 *
 *   Configure Port B pins for use by UART 1 as follows:
 *   RB5 - PWM output for WS2812 data (output)
 *
 ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "config.h"
#include "ws2812.h"
#include "../../Libraries/PPSLIB/pps.h"
#include "../../Libraries/INTLIB/intlib.h"

#define WS2812_PWM_CLOCK_HZ _XTAL_FREQ
#define WS2812_BIT_RATE_HZ 800000UL
#define WS2812_BIT_TIME_NS 1250UL
#define WS2812_T0H_NS 400UL
#define WS2812_T1H_NS 800UL
#define WS2812_PWM_PERIOD_TICKS ((uint16_t)((WS2812_PWM_CLOCK_HZ / WS2812_BIT_RATE_HZ) - 1UL))
#define WS2812_PWM_PERIOD_COUNTS (WS2812_PWM_PERIOD_TICKS + 1U)
#define WS2812_DUTY_0_TICKS ((uint8_t)(((WS2812_PWM_PERIOD_COUNTS * WS2812_T0H_NS) + (WS2812_BIT_TIME_NS / 2UL)) / WS2812_BIT_TIME_NS))
#define WS2812_DUTY_1_TICKS ((uint8_t)(((WS2812_PWM_PERIOD_COUNTS * WS2812_T1H_NS) + (WS2812_BIT_TIME_NS / 2UL)) / WS2812_BIT_TIME_NS))
#define WS2812_PWM_CLOCK_SOURCE 0b00010U
#define WS2812_RESET_PERIODS 40U
#define WS2812_BITS_PER_LED 24U
#define WS2812_BYTES_PER_LED WS2812_BITS_PER_LED
#define WS2812_DMA_BUFFER_SIZE ((uint16_t)(WS2812_MAX_LEDS * WS2812_BYTES_PER_LED + WS2812_RESET_PERIODS))

static uint8_t ws2812_dmaBuffer[WS2812_DMA_BUFFER_SIZE];
static WS2812_Strip_t *s_activeStrip[WS2812_MAX_PWM_MODULES] = {(WS2812_Strip_t *) 0};
static uint16_t ws2812_activeLength[WS2812_MAX_PWM_MODULES] = {0U, 0U, 0U};
static uint16_t ws2812_activeIndex[WS2812_MAX_PWM_MODULES] = {0U, 0U, 0U};

/// @brief Sets up the PWM data pin for the WS2812 strip.
/// @param dataPin The data pin to configure for PWM output.
/// @param pwmMapping The PWM output PPS mapping value for the specified data pin.
/// @return WS2812_Status_t indicating success or failure of the operation.

static WS2812_Status_t setupPWMDataPin(WS2812_DATA_PIN dataPin, uint8_t pwmMapping) {
    switch (dataPin) {
        case WS2812_PIN_RB0:
            TRISBbits.TRISB0 = 0U;
            RB0PPS = pwmMapping;
            break;
        case WS2812_PIN_RB1:
            TRISBbits.TRISB1 = 0U;
            RB1PPS = pwmMapping;
            break;
        case WS2812_PIN_RB2:
            TRISBbits.TRISB2 = 0U;
            RB2PPS = pwmMapping;
            break;
        case WS2812_PIN_RB3:
            TRISBbits.TRISB3 = 0U;
            RB3PPS = pwmMapping;
            break;
        case WS2812_PIN_RB4:
            TRISBbits.TRISB4 = 0U;
            RB4PPS = pwmMapping;
            break;
        case WS2812_PIN_RB5:
            TRISBbits.TRISB5 = 0U;
            RB5PPS = pwmMapping;
            break;
        case WS2812_PIN_RB6:
            TRISBbits.TRISB6 = 0U;
            RB6PPS = pwmMapping;
            break;
        case WS2812_PIN_RB7:
            TRISBbits.TRISB7 = 0U;
            RB7PPS = pwmMapping;
            break;
        case WS2812_PIN_RC0:
            TRISCbits.TRISC0 = 0U;
            RC0PPS = pwmMapping;
            break;
        case WS2812_PIN_RC1:
            TRISCbits.TRISC1 = 0U;
            RC1PPS = pwmMapping;
            break;
        case WS2812_PIN_RC2:
            TRISCbits.TRISC2 = 0U;
            RC2PPS = pwmMapping;
            break;
        case WS2812_PIN_RC3:
            TRISCbits.TRISC3 = 0U;
            RC3PPS = pwmMapping;
            break;
        case WS2812_PIN_RC4:
            TRISCbits.TRISC4 = 0U;
            RC4PPS = pwmMapping;
            break;
        case WS2812_PIN_RC5:
            TRISCbits.TRISC5 = 0U;
            RC5PPS = pwmMapping;
            break;
        case WS2812_PIN_RC6:
            TRISCbits.TRISC6 = 0U;
            RC6PPS = pwmMapping;
            break;
        case WS2812_PIN_RC7:
            TRISCbits.TRISC7 = 0U;
            RC7PPS = pwmMapping;
            break;
        default:
            return WS2812_INVALID_PARAM;
    }

    return WS2812_OK;
}

static volatile uint16_t *getPWMDutyRegister(WS2812_PWM_Module_t module) {
    switch (module) {
        case WS2812_PWM_MODULE_1:
            return &PWM1S1P1;
        case WS2812_PWM_MODULE_2:
            return &PWM2S1P1;
        case WS2812_PWM_MODULE_3:
            return &PWM3S1P1;
        default:
            return (volatile uint16_t *)0;
    }
}

static uint8_t getPWMPeriodTrigger(WS2812_PWM_Module_t module) {
    switch (module) {
        case WS2812_PWM_MODULE_1:
            return (uint8_t) IRQ_PWM1PR;
        case WS2812_PWM_MODULE_2:
            return (uint8_t) IRQ_PWM2PR;
        case WS2812_PWM_MODULE_3:
            return (uint8_t) IRQ_PWM3PR;
        default:
            return 0U;
    }
}

static volatile uint16_t *getPWMCompareRegister(WS2812_PWM_Module_t module) {
    switch (module) {
        case WS2812_PWM_MODULE_1:
            return &PWM1S1P1;
        case WS2812_PWM_MODULE_2:
            return &PWM2S1P1;
        case WS2812_PWM_MODULE_3:
            return &PWM3S1P1;
        default:
            return (volatile uint16_t *)0;
    }
}

static void clearPWMInterruptFlags(WS2812_PWM_Module_t module) {
    switch (module) {
        case WS2812_PWM_MODULE_1:
            PIR4bits.PWM1PIF = 0U;
            PIR4bits.PWM1IF = 0U;
            break;
        case WS2812_PWM_MODULE_2:
            PIR5bits.PWM2PIF = 0U;
            PIR5bits.PWM2IF = 0U;
            break;
        case WS2812_PWM_MODULE_3:
            PIR7bits.PWM3PIF = 0U;
            PIR7bits.PWM3IF = 0U;
            break;
        default:
            break;
    }
}

static void commitPWMUpdate(WS2812_PWM_Module_t module) {
    switch (module) {
        case WS2812_PWM_MODULE_1:
            PWM1CONbits.LD = 1U;
            break;
        case WS2812_PWM_MODULE_2:
            PWM2CONbits.LD = 1U;
            break;
        case WS2812_PWM_MODULE_3:
            PWM3CONbits.LD = 1U;
            break;
        default:
            break;
    }
}

/// @brief Disables the specified PWM module.
/// @param module   The PWM module to disable.
/// @return None

static void disablePWMModule(WS2812_PWM_Module_t module) {
    switch (module) {
        case WS2812_PWM_MODULE_1:
            PWM1CONbits.EN = 0U;
            PWM1GIEbits.S1P1IE = 0U;
            PWM1GIRbits.S1P1IF = 0U;
            PIE4bits.PWM1PIE = 0U;
            PIE4bits.PWM1IE = 0U;
            clearPWMInterruptFlags(module);
            break;
        case WS2812_PWM_MODULE_2:
            PWM2CONbits.EN = 0U;
            PWM2GIEbits.S1P1IE = 0U;
            PWM2GIRbits.S1P1IF = 0U;
            PIE5bits.PWM2PIE = 0U;
            PIE5bits.PWM2IE = 0U;
            clearPWMInterruptFlags(module);
            break;
        case WS2812_PWM_MODULE_3:
            PWM3CONbits.EN = 0U;
            PWM3GIEbits.S1P1IE = 0U;
            PWM3GIRbits.S1P1IF = 0U;
            PIE7bits.PWM3PIE = 0U;
            PIE7bits.PWM3IE = 0U;
            clearPWMInterruptFlags(module);
            break;
        default:
            break;
    }
}

/// @brief Enables the specified PWM module.
/// @param module The PWM module to enable.
/// @return None

static void enablePWMModule(WS2812_PWM_Module_t module) {
    switch (module) {
        case WS2812_PWM_MODULE_1:
            clearPWMInterruptFlags(module);
            PIE4bits.PWM1PIE = 1U; // Enable period interrupt for WS2812 bit pacing
            PIE4bits.PWM1IE = 0U; // Keep module/parameter interrupt disabled
            PWM1GIEbits.S1P1IE = 0U; // Do not use slice parameter interrupt for bit stepping
            PWM1CONbits.EN = 1U; // Enable the PWM module
            break;
        case WS2812_PWM_MODULE_2:
            clearPWMInterruptFlags(module);
            PIE5bits.PWM2PIE = 1U; // Enable period interrupt for WS2812 bit pacing
            PIE5bits.PWM2IE = 0U; // Keep module/parameter interrupt disabled
            PWM2GIEbits.S1P1IE = 0U; // Do not use slice parameter interrupt for bit stepping
            PWM2CONbits.EN = 1U; // Enable the PWM module
            break;
        case WS2812_PWM_MODULE_3:
            clearPWMInterruptFlags(module);
            PIE7bits.PWM3PIE = 1U; // Enable period interrupt for WS2812 bit pacing
            PIE7bits.PWM3IE = 0U; // Keep module/parameter interrupt disabled
            PWM3GIEbits.S1P1IE = 0U; // Do not use slice parameter interrupt for bit stepping
            PWM3CONbits.EN = 1U; // Enable the PWM module
            break;
        default:
            break;
    }
}

static void finalizePWMTransfer(WS2812_Strip_t *strip) {
    if (strip != NULL) {
        strip->busy = false;
    }
    disablePWMModule(strip->pwmModule);
    s_activeStrip[strip->pwmModule] = (WS2812_Strip_t *) 0;
}

static WS2812_Status_t configurePWMModule(WS2812_PWM_Module_t module) {
    switch (module) {
        case WS2812_PWM_MODULE_1:
            PWM1CONbits.EN = 0U; // Disable the PWM module
            PWM1ERSbits.ERS = 0U; // No external reset source
            PWM1CLKbits.CLK = WS2812_PWM_CLOCK_SOURCE; // Clock source
            PWM1LDSbits.LDS = 0U; // No auto load used
            PWM1PR = WS2812_PWM_PERIOD_TICKS; // Clock ticks per period
            PWM1CPRE = 0U; // No prescaler counts
            PWM1PIPOS = 0U; // No period interrupt post scaler
            PWM1GIEbits.S1P1IE = 0U; // Disable PWM1 interrupt for S1P1
            PWM1GIEbits.S1P2IE = 0U; // Disable PWM1 interrupt for S1P2
            PWM1GIRbits.S1P1IF = 0U; // Clear PWM1 interrupt flag for S1P1
            PWM1GIEbits.S1P2IE = 0U; // Disable PWM1 interrupt for S1P2
            PIR4bits.PWM1PIF = 0U; // Clear PWM1 period interrupt flag
            PIR4bits.PWM1IF = 0U; // Clear PWM1 module interrupt flag
            PWM1S1CFGbits.MODE = WS2812_PWM_MODE; // Set PWM1 S1 configuration mode
            PWM1S1CFGbits.PPEN = 0U; // No push-pull needed
            PWM1S1CFGbits.POL1 = 0U; // Set polarity for S1P1
            PWM1S1CFGbits.POL2 = 0U; // Set polarity for S1P2
            PWM1CONbits.ERSNOW = 0U; // External reset not used
            PWM1CONbits.ERSPOL = 0U; // External reset is actually not used
            PWM1CONbits.LD = 1U; // Load the new PWM period value into the module
            break;
        case WS2812_PWM_MODULE_2:
            PWM2CONbits.EN = 0U; // Disable the PWM module
            PWM2ERSbits.ERS = 0U; // No external reset source
            PWM2CLKbits.CLK = WS2812_PWM_CLOCK_SOURCE; // Clock source
            PWM2LDSbits.LDS = 0U; // No auto load used
            PWM2PR = WS2812_PWM_PERIOD_TICKS; // Clock ticks per period
            PWM2CPRE = 0U; // No prescaler counts
            PWM2PIPOS = 0U; // No period interrupt post scaler
            PWM2GIEbits.S1P1IE = 0U; // Disable PWM1 interrupt for S1P1
            PWM2GIEbits.S1P2IE = 0U; // Disable PWM1 interrupt for S1P2
            PWM2GIRbits.S1P1IF = 0U; // Clear PWM1 interrupt flag for S1P1
            PWM2GIEbits.S1P2IE = 0U; // Disable PWM1 interrupt for S1P2
            PIR5bits.PWM2PIF = 0U; // Clear PWM2 period interrupt flag
            PIR5bits.PWM2IF = 0U; // Clear PWM2 module interrupt flag
            PWM2S1CFGbits.MODE = WS2812_PWM_MODE; // Set PWM1 S1 configuration mode
            PWM2S1CFGbits.PPEN = 0U; // No push-pull needed
            PWM2S1CFGbits.POL1 = 0U; // Set polarity for S1P1
            PWM2S1CFGbits.POL2 = 0U; // Set polarity for S1P2
            PWM2CONbits.ERSNOW = 0U; // External reset not used
            PWM2CONbits.ERSPOL = 0U; // External reset is actually not used
            PWM2CONbits.LD = 1U; // Load the new PWM period value into the module
            break;
        case WS2812_PWM_MODULE_3:
            PWM3CONbits.EN = 0U; // Disable the PWM module
            PWM3ERSbits.ERS = 0U; // No external reset source
            PWM3CLKbits.CLK = WS2812_PWM_CLOCK_SOURCE; // Clock source
            PWM3LDSbits.LDS = 0U; // No auto load used
            PWM3PR = WS2812_PWM_PERIOD_TICKS; // Clock ticks per period
            PWM3CPRE = 0U; // No prescaler counts
            PWM3PIPOS = 0U; // No period interrupt post scaler
            PWM3GIEbits.S1P1IE = 0U; // Disable PWM3 interrupt for S1P1
            PWM3GIEbits.S1P2IE = 0U; // Disable PWM3 interrupt for S1P2
            PWM3GIRbits.S1P1IF = 0U; // Clear PWM3 interrupt flag for S1P1
            PWM3GIEbits.S1P2IE = 0U; // Disable PWM3 interrupt for S1P2
            PIR7bits.PWM3PIF = 0U; // Clear PWM3 period interrupt flag
            PIR7bits.PWM3IF = 0U; // Clear PWM3 module interrupt flag
            PWM3S1CFGbits.MODE = WS2812_PWM_MODE; // Set PWM3 S1 configuration mode
            PWM3S1CFGbits.PPEN = 0U; // No push-pull needed
            PWM3S1CFGbits.POL1 = 0U; // Set polarity for S1P1
            PWM3S1CFGbits.POL2 = 0U; // Set polarity for S1P2
            PWM3CONbits.ERSNOW = 0U; // External reset not used
            PWM3CONbits.ERSPOL = 0U; // External reset is actually not used
            PWM3CONbits.LD = 1U; // Load the new PWM period value into the module
            break;
        default:
            return WS2812_INVALID_PARAM;
    }

    return WS2812_OK;
}

/// @brief The waveform buffer is actually an array of duty cycle times for each bit to be
/// sent to the WS2812's in the strip.  Each 1 bit in the color value is represented by a
/// @note The specific duty cycle for a '1' bit is defined by WS2812_DUTY_1_TICKS and for
/// a '0' bit by WS2812_DUTY_0_TICKS.
/// @note The buffer should be large enough to hold all the duty cycles for the entire strip.
/// This means for a 30 LED strip, where there are 24 bits for the colors per LED, the
/// waveform buffer should have at least 30 * 24 entries, plus additional entries for the
/// reset period.
/// @param strip The WS2812 strip structure to have the waveform calculated.
/// @param buffer The buffer to store the waveform duty cycle values.
/// @return The number of entries written to the buffer, or 0 if the buffer was too small.

static uint16_t buildWaveformBuffer(const WS2812_Strip_t *strip, uint8_t *buffer) {
    uint16_t bufferIndex = 0U;

    for (uint16_t led = 0U; led < strip->numLEDs; led++) {
        const WS2812_Color_t color = strip->colors[led];
        const uint8_t colorOrder[3] = {color.green, color.red, color.blue};

        // Output duty cycle times for each color channel (green, red, blue) of the
        // current LED.
        for (uint8_t channel = 0U; channel < 3U; channel++) {
            // Output duty cycle times for each bit of the current color channel.
            for (int8_t bit = 7; bit >= 0; bit--) {
                if (bufferIndex >= WS2812_DMA_BUFFER_SIZE) {
                    return 0U;
                }

                uint8_t bitValue = (colorOrder[channel] & (uint8_t) (1U << bit)) != 0U;
                buffer[bufferIndex++] = bitValue ? WS2812_DUTY_1_TICKS : WS2812_DUTY_0_TICKS;
            }
        }
    }

    // Append the reset period to the waveform buffer.
    for (uint16_t resetPeriod = 0U; resetPeriod < WS2812_RESET_PERIODS; resetPeriod++) {
        if (bufferIndex >= WS2812_DMA_BUFFER_SIZE) {
            return 0U;
        }

        buffer[bufferIndex++] = 0U;
    }

    return bufferIndex;
}

/// @brief Function to initialize the WS2812 strip with the specified data pin and
/// number of LEDs.
/// @param strip The WS2812 strip structure to initialize.
/// @param module The PWM module to use for data transmission.
/// @param dataPin  The data pin to use for the WS2812 strip.
/// @param numLEDs  The number of LEDs in the strip.
/// @return WS2812_Status_t indicating success or failure of the operation.

WS2812_Status_t WS2812_Init(WS2812_Strip_t *strip, WS2812_PWM_Module_t module,
        WS2812_DATA_PIN dataPin, uint16_t numLEDs) {
    printf("Duty cycle tick counts: 0: %d, 1:%d%s", WS2812_DUTY_0_TICKS, WS2812_DUTY_1_TICKS, CRLF);


    if (strip == NULL || numLEDs == 0U || numLEDs > WS2812_MAX_LEDS) {
        return WS2812_INVALID_PARAM;
    }

    if (strip->initialized) {
        return WS2812_ERROR;
    }

    strip->signature = WS2812_SIGNATURE;
    strip->initialized = true;
    strip->busy = false;
    strip->numLEDs = numLEDs;
    strip->pwmModule = module;
    strip->dataPin = dataPin;

    WS2812_Status_t status = WS2812_OK;

    INTCON0bits.GIEH = 0U;
    INTCON0bits.GIEL = 0U;

    // Setup PWM1, 2, and 3 interrupts as high priority 
    IPR4bits.PWM1PIP = 1U; // Set PWM1 period interrupt as high priority
    IPR4bits.PWM1IP = 1U;  // Set PWM1 parameter interrupt as high priority
    IPR5bits.PWM2PIP = 1U; // Set PWM2 period interrupt as high priority
    IPR5bits.PWM2IP = 1U;  // Set PWM2 parameter interrupt as high priority
    IPR7bits.PWM3PIP = 1U; // Set PWM3 period interrupt as high priority
    IPR7bits.PWM3IP = 1U;  // Set PWM3 parameter interrupt as high priority

    PPS_Unlock();
    switch (strip->pwmModule) {
        case WS2812_PWM_MODULE_1:
            status = setupPWMDataPin(strip->dataPin, 0x18U);
            break;
        case WS2812_PWM_MODULE_2:
            status = setupPWMDataPin(strip->dataPin, 0x1AU);
            break;
        case WS2812_PWM_MODULE_3:
            status = setupPWMDataPin(strip->dataPin, 0x1CU);
            break;
        default:
            PPS_Lock();
            INTCON0bits.GIEH = 1;
            INTCON0bits.GIEL = 1;
            return WS2812_INVALID_PARAM;
    }
    PPS_Lock();
    INTCON0bits.GIEH = 1;
    INTCON0bits.GIEL = 1;

    if (status != WS2812_OK) {
        return status;
    }

    status = configurePWMModule(strip->pwmModule);
    if (status != WS2812_OK) {
        return status;
    }

    clearPWMInterruptFlags(strip->pwmModule);
    PIE4bits.PWM1PIE = 0U;
    PIE4bits.PWM1IE = 0U;
    PIE5bits.PWM2PIE = 0U;
    PIE5bits.PWM2IE = 0U;
    PIE7bits.PWM3PIE = 0U;
    PIE7bits.PWM3IE = 0U;

    disablePWMModule(strip->pwmModule);
    return WS2812_OK;
}

/// @brief Sets the color of the specified LED in the WS2812 strip.
/// @param strip  The WS2812 strip structure.
/// @param index  The index of the LED to set the color for (0-based).
/// @param color  The color to set for the specified LED.
/// @return  WS2812_Status_t indicating success or failure of the operation.

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

/// @brief Updates the WS2812 strip with the current color values for each LED.
/// @param strip  The WS2812 strip structure to update.
/// @return  WS2812_Status_t indicating success or failure of the operation.

WS2812_Status_t WS2812_Update(WS2812_Strip_t *strip) {
    if (strip == NULL || !strip->initialized) {
        return WS2812_NOT_INITIALIZED;
    }

    if (strip->busy) {
        return WS2812_BUSY;
    }
    volatile uint16_t *dutyRegister = getPWMDutyRegister(strip->pwmModule);
    uint8_t dmaTrigger = getPWMPeriodTrigger(strip->pwmModule);
    if ((dutyRegister == 0) || (dmaTrigger == 0U)) {
        return WS2812_INVALID_PARAM;
    }

    uint16_t waveformLength = buildWaveformBuffer(strip, ws2812_dmaBuffer);
    if (waveformLength == 0U) {
        return WS2812_ERROR;
    }

    strip->busy = true;
    s_activeStrip[strip->pwmModule] = strip;
    ws2812_activeLength[strip->pwmModule] = waveformLength;
    ws2812_activeIndex[strip->pwmModule] = 1U;
    disablePWMModule(strip->pwmModule);

    *dutyRegister = ws2812_dmaBuffer[0U];
    commitPWMUpdate(strip->pwmModule);
    if (waveformLength == 1U) {
        finalizePWMTransfer(strip);
        return WS2812_OK;
    }

    enablePWMModule(strip->pwmModule);

    return WS2812_OK;
}

static void WS2812_ServicePwmInterrupt(WS2812_PWM_Module_t module) {
    WS2812_Strip_t *strip = s_activeStrip[module];
    volatile uint16_t *dutyRegister = getPWMCompareRegister(module);

    if ((strip == NULL) || (dutyRegister == NULL)) {
        return;
    }

    clearPWMInterruptFlags(module);

    if (ws2812_activeIndex[module] < ws2812_activeLength[module]) {
        *dutyRegister = ws2812_dmaBuffer[ws2812_activeIndex[module]];
        commitPWMUpdate(module);
        ws2812_activeIndex[module]++;
        return;
    }

    finalizePWMTransfer(strip);
}

/// @brief PWM1 period interrupt service routine for WS2812 output.

void __interrupt(irq(IRQ_PWM1PR), high_priority) WS2812_PWM1_ISR(void) {
    WS2812_ServicePwmInterrupt(WS2812_PWM_MODULE_1);
}

/// @brief PWM1 parameter interrupt service routine for WS2812 output.

void __interrupt(irq(IRQ_PWM1), high_priority) WS2812_PWM1_PARAM_ISR(void) {
    // Parameter interrupt is intentionally not used to clock WS2812 bits.
    // If it ever fires, just clear flags to avoid stale interrupt state.
    clearPWMInterruptFlags(WS2812_PWM_MODULE_1);
}


/// @brief PWM2 interrupt service routine for WS2812 output.

void __interrupt(irq(IRQ_PWM2PR), high_priority) WS2812_PWM2_ISR(void) {
    WS2812_ServicePwmInterrupt(WS2812_PWM_MODULE_2);
}

/// @brief PWM3 interrupt service routine for WS2812 output.

void __interrupt(irq(IRQ_PWM3PR), high_priority) WS2812_PWM3_ISR(void) {
    WS2812_ServicePwmInterrupt(WS2812_PWM_MODULE_3);
}

/// @brief Clears all LEDs to a color of (0, 0, 0) and updates the strip.
/// @param strip  The WS2812 strip structure to clear.
/// @return  WS2812_Status_t indicating success or failure of the operation.

WS2812_Status_t WS2812_Clear(WS2812_Strip_t *strip) {
    if (strip == NULL || !strip->initialized) {
        return WS2812_NOT_INITIALIZED;
    }

    for (uint16_t i = 0U; i < strip->numLEDs; i++) {
        strip->colors[i] = (WS2812_Color_t){0U, 0U, 0U};
    }

    return WS2812_Update(strip);
}

/// @brief  Checks if the WS2812 strip is currently busy updating.
/// @param strip  The WS2812 strip structure to check.
/// @return  WS2812_Status_t indicating if the strip is busy or not.

WS2812_Status_t WS2812_isBusy(WS2812_Strip_t *strip) {
    if (strip == NULL || !strip->initialized) {
        return WS2812_NOT_INITIALIZED;
    }

    if (strip->busy) {
        return WS2812_BUSY;
    }

    return WS2812_OK;
}
