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

#include "config.h"
#include "ws2812.h"
#include "../../Libraries/PPSLIB/pps.h"
#include "../../Libraries/INTLIB/intlib.h"
#include "../../Libraries/DMALIB/dmalib.h"

#define WS2812_DMA_CHANNEL 1U
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
static WS2812_Strip_t *s_activeStrip[WS2812_MAX_PWM_MODULES] = { (WS2812_Strip_t *)0 };

/// @brief Sets up the PWM data pin for the WS2812 strip.
/// @param dataPin The data pin to configure for PWM output.
/// @param pwmMapping The PWM output PPS mapping value for the specified data pin.
/// @return WS2812_Status_t indicating success or failure of the operation.
static WS2812_Status_t setupPWMDataPin(WS2812_DATA_PIN dataPin, uint8_t pwmMapping)
{
    switch (dataPin)
    {
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

static volatile uint8_t *getPWMDutyRegister(WS2812_PWM_Module_t module)
{
    switch (module)
    {
    case WS2812_PWM_MODULE_1:
        return &PWM1S1P1L;
    case WS2812_PWM_MODULE_2:
        return &PWM2S1P1L;
    case WS2812_PWM_MODULE_3:
        return &PWM3S1P1L;
    default:
        return (volatile uint8_t *)0;
    }
}

static uint8_t getPWMPeriodTrigger(WS2812_PWM_Module_t module)
{
    switch (module)
    {
    case WS2812_PWM_MODULE_1:
        return (uint8_t)IRQ_PWM1PR;
    case WS2812_PWM_MODULE_2:
        return (uint8_t)IRQ_PWM2PR;
    case WS2812_PWM_MODULE_3:
        return (uint8_t)IRQ_PWM3PR;
    default:
        return 0U;
    }
}

static void disablePWMModule(WS2812_PWM_Module_t module)
{
    switch (module)
    {
    case WS2812_PWM_MODULE_1:
        PWM1CONbits.EN = 0U;
        PWM1GIEbits.S1P1IE = 0U;
        PWM1GIRbits.S1P1IF = 0U;
        PWM1S1P1L = 0U;
        PWM1S1P1H = 0U;
        break;
    case WS2812_PWM_MODULE_2:
        PWM2CONbits.EN = 0U;
        PWM2GIEbits.S1P1IE = 0U;
        PWM2GIRbits.S1P1IF = 0U;
        PWM2S1P1L = 0U;
        PWM2S1P1H = 0U;
        break;
    case WS2812_PWM_MODULE_3:
        PWM3CONbits.EN = 0U;
        PWM3GIEbits.S1P1IE = 0U;
        PWM3GIRbits.S1P1IF = 0U;
        PWM3S1P1L = 0U;
        PWM3S1P1H = 0U;
        break;
    default:
        break;
    }
}

static void finalizePWMTransfer(WS2812_Strip_t *strip, volatile uint8_t *dutyRegister)
{
    DMA_SelectChannel(WS2812_DMA_CHANNEL);
    DMAnCON0bits.DGO = 0U;
    DMAnCON0bits.EN = 0U;
    PIE2bits.DMA1SCNTIE = 0U;
    PIR2bits.DMA1SCNTIF = 0U;
    if (strip != NULL)
    {
        strip->busy = false;
    }

    s_activeStrip[strip->pwmModule] = (WS2812_Strip_t *)0;
}

static WS2812_Status_t configurePWMModule(WS2812_PWM_Module_t module)
{
    switch (module)
    {
    case WS2812_PWM_MODULE_1:
        PWM1CONbits.EN = 0U;                       // Disable the PWM module
        PWM1ERSbits.ERS = 0U;                      // No external reset source
        PWM1CLKbits.CLK = WS2812_PWM_CLOCK_SOURCE; // Clock source
        PWM1LDSbits.LDS = 0U;                      // No auto load used
        PWM1PR = WS2812_PWM_PERIOD_TICKS;          // Clock ticks per period
        PWM1CPRE = 0U;                             // No prescaler counts
        PWM1PIPOS = 0U;                            // No period interrupt post scaler
        PWM1GIEbits.S1P1IE = 0U;                   // Disable PWM1 interrupt for S1P1
        PWM1GIEbits.S1P2IE = 0U;                   // Disable PWM1 interrupt for S1P2
        PWM1GIRbits.S1P1IF = 0U;                   // Clear PWM1 interrupt flag for S1P1
        PWM1GIEbits.S1P2IE = 0U;                   // Disable PWM1 interrupt for S1P2
        PWM1S1CFGbits.MODE = WS2812_PWM_MODE;      // Set PWM1 S1 configuration mode
        PWM1S1CFGbits.PPEN = 0U;                   // No push-pull needed
        PWM1S1CFGbits.POL1 = 0U;                   // Set polarity for S1P1
        PWM1S1CFGbits.POL2 = 0U;                   // Set polarity for S1P2
        PWM1CONbits.ERSNOW = 0U;                   // Stop counter at end of period
        PWM1CONbits.ERSPOL = 0U;                   // External reset is actually not used
        PWM1CONbits.LD = 1U;                       // Load the new PWM period value into the module
        break;
    case WS2812_PWM_MODULE_2:
        PWM2CONbits.EN = 0U;                       // Disable the PWM module
        PWM2ERSbits.ERS = 0U;                      // No external reset source
        PWM2CLKbits.CLK = WS2812_PWM_CLOCK_SOURCE; // Clock source
        PWM2LDSbits.LDS = 0U;                      // No auto load used
        PWM2PR = WS2812_PWM_PERIOD_TICKS;          // Clock ticks per period
        PWM2CPRE = 0U;                             // No prescaler counts
        PWM2PIPOS = 0U;                            // No period interrupt post scaler
        PWM2GIEbits.S1P1IE = 0U;                   // Disable PWM1 interrupt for S1P1
        PWM2GIEbits.S1P2IE = 0U;                   // Disable PWM1 interrupt for S1P2
        PWM2GIRbits.S1P1IF = 0U;                   // Clear PWM1 interrupt flag for S1P1
        PWM2GIEbits.S1P2IE = 0U;                   // Disable PWM1 interrupt for S1P2
        PWM2S1CFGbits.MODE = WS2812_PWM_MODE;      // Set PWM1 S1 configuration mode
        PWM2S1CFGbits.PPEN = 0U;                   // No push-pull needed
        PWM2S1CFGbits.POL1 = 0U;                   // Set polarity for S1P1
        PWM2S1CFGbits.POL2 = 0U;                   // Set polarity for S1P2
        PWM2CONbits.ERSNOW = 0U;                   // Stop counter at end of period
        PWM2CONbits.ERSPOL = 0U;                   // External reset is actually not used
        PWM2CONbits.LD = 1U;                       // Load the new PWM period value into the module
        break;
    case WS2812_PWM_MODULE_3:
        PWM3CONbits.EN = 0U;                       // Disable the PWM module
        PWM3ERSbits.ERS = 0U;                      // No external reset source
        PWM3CLKbits.CLK = WS2812_PWM_CLOCK_SOURCE; // Clock source
        PWM3LDSbits.LDS = 0U;                      // No auto load used
        PWM3PR = WS2812_PWM_PERIOD_TICKS;          // Clock ticks per period
        PWM3CPRE = 0U;                             // No prescaler counts
        PWM3PIPOS = 0U;                            // No period interrupt post scaler
        PWM3GIEbits.S1P1IE = 0U;                   // Disable PWM3 interrupt for S1P1
        PWM3GIEbits.S1P2IE = 0U;                   // Disable PWM3 interrupt for S1P2
        PWM3GIRbits.S1P1IF = 0U;                   // Clear PWM3 interrupt flag for S1P1
        PWM3GIEbits.S1P2IE = 0U;                   // Disable PWM3 interrupt for S1P2
        PWM3S1CFGbits.MODE = WS2812_PWM_MODE;      // Set PWM3 S1 configuration mode
        PWM3S1CFGbits.PPEN = 0U;                   // No push-pull needed
        PWM3S1CFGbits.POL1 = 0U;                   // Set polarity for S1P1
        PWM3S1CFGbits.POL2 = 0U;                   // Set polarity for S1P2
        PWM3CONbits.ERSNOW = 0U;                   // Stop counter at end of period
        PWM3CONbits.ERSPOL = 0U;                   // External reset is actually not used
        PWM3CONbits.LD = 1U;                       // Load the new PWM period value into the module
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
static uint16_t buildWaveformBuffer(const WS2812_Strip_t *strip, uint8_t *buffer)
{
    uint16_t bufferIndex = 0U;

    for (uint16_t led = 0U; led < strip->numLEDs; led++)
    {
        const WS2812_Color_t color = strip->colors[led];
        const uint8_t colorOrder[3] = {color.green, color.red, color.blue};

        // Output duty cycle times for each color channel (green, red, blue) of the 
        // current LED.
        for (uint8_t channel = 0U; channel < 3U; channel++)
        {
            // Output duty cycle times for each bit of the current color channel.
            for (int8_t bit = 7; bit >= 0; bit--)
            {
                if (bufferIndex >= WS2812_DMA_BUFFER_SIZE)
                {
                    return 0U;
                }

                buffer[bufferIndex++] = ((colorOrder[channel] & (uint8_t)(1U << bit)) != 0U)
                                            ? WS2812_DUTY_1_TICKS
                                            : WS2812_DUTY_0_TICKS;
            }
        }
    }

    // Append the reset period to the waveform buffer.
    for (uint16_t resetPeriod = 0U; resetPeriod < WS2812_RESET_PERIODS; resetPeriod++)
    {
        if (bufferIndex >= WS2812_DMA_BUFFER_SIZE)
        {
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
                            WS2812_DATA_PIN dataPin, uint16_t numLEDs)
{
    if (strip == NULL || numLEDs == 0U || numLEDs > WS2812_MAX_LEDS)
    {
        return WS2812_INVALID_PARAM;
    }

    if (strip->initialized)
    {
        return WS2812_ERROR;
    }

    strip->signature = WS2812_SIGNATURE;
    strip->initialized = true;
    strip->busy = false;
    strip->numLEDs = numLEDs;
    strip->pwmModule = module;
    strip->dataPin = dataPin;

    WS2812_Status_t status = WS2812_OK;

    uint8_t savedGIEH = INTCON0bits.GIEH;
    uint8_t savedGIEL = INTCON0bits.GIEL;

    INTCON0bits.GIEH = 0U;
    INTCON0bits.GIEL = 0U;
    PPS_Unlock();
    switch (strip->pwmModule)
    {
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
        INTCON0bits.GIEH = savedGIEH;
        INTCON0bits.GIEL = savedGIEL;
        return WS2812_INVALID_PARAM;
    }
    PPS_Lock();
    INTCON0bits.GIEH = savedGIEH;
    INTCON0bits.GIEL = savedGIEL;

    if (status != WS2812_OK)
    {
        return status;
    }

    status = configurePWMModule(strip->pwmModule);
    if (status != WS2812_OK)
    {
        return status;
    }

    // To reduce softwaare overhead, this module uses the DMA module chosen to transfer
    // the waveform buffer.  This eliminates all application involvement in the process,
    // as well as the driver.

    // Configure DMA1 source-count interrupt once for interrupt-driven completion.
    IPR2bits.DMA1SCNTIP = 0U;       // Clear the DMA1 source-count interrupt priority bit
    PIR2bits.DMA1SCNTIF = 0U;       // Clear the DMA1 source-count interrupt flag
    PIE2bits.DMA1SCNTIE = 0U;       // Disable the DMA1 source-count interrupt

    disablePWMModule(strip->pwmModule);
    return WS2812_OK;
}

/// @brief Sets the color of the specified LED in the WS2812 strip.
/// @param strip  The WS2812 strip structure.
/// @param index  The index of the LED to set the color for (0-based).
/// @param color  The color to set for the specified LED.
/// @return  WS2812_Status_t indicating success or failure of the operation.
WS2812_Status_t WS2812_SetColor(WS2812_Strip_t *strip, uint16_t index, WS2812_Color_t color)
{
    if (strip == NULL || !strip->initialized)
    {
        return WS2812_NOT_INITIALIZED;
    }

    if (index >= strip->numLEDs)
    {
        return WS2812_OUT_OF_BOUNDS;
    }

    strip->colors[index] = color;
    return WS2812_OK;
}

/// @brief Updates the WS2812 strip with the current color values for each LED.
/// @param strip  The WS2812 strip structure to update.
/// @return  WS2812_Status_t indicating success or failure of the operation.
WS2812_Status_t WS2812_Update(WS2812_Strip_t *strip)
{
    if (strip == NULL || !strip->initialized)
    {
        return WS2812_NOT_INITIALIZED;
    }

    if (strip->busy)
    {
        return WS2812_BUSY;
    }

    volatile uint8_t *dutyRegister = getPWMDutyRegister(strip->pwmModule);
    uint8_t dmaTrigger = getPWMPeriodTrigger(strip->pwmModule);
    if ((dutyRegister == 0) || (dmaTrigger == 0U))
    {
        return WS2812_INVALID_PARAM;
    }

    uint16_t waveformLength = buildWaveformBuffer(strip, ws2812_dmaBuffer);
    if (waveformLength == 0U)
    {
        return WS2812_ERROR;
    }

    strip->busy = true;
    s_activeStrip[strip->pwmModule] = strip;
    disablePWMModule(strip->pwmModule);

    *dutyRegister = ws2812_dmaBuffer[0U];
    if (waveformLength == 1U)
    {
        strip->busy = false;
        return WS2812_OK;
    }

    DMA_SelectChannel(WS2812_DMA_CHANNEL);
    DMAnCON0bits.DGO = 0U;
    DMAnCON0bits.EN = 0U;
    DMAnCON0bits.SIRQEN = 1U;
    DMAnCON0bits.AIRQEN = 0U;
    DMAnCON1bits.SMODE = 1U;
    DMAnCON1bits.DMODE = 0U;
    DMAnCON1bits.SSTP = 1U;
    DMAnSIRQ = dmaTrigger;
    DMAnAIRQ = 0U;
    DMA_SetSourceAddress(WS2812_DMA_CHANNEL, &ws2812_dmaBuffer[1U]);
    DMA_SetDestAddress(WS2812_DMA_CHANNEL, dutyRegister);
    DMA_SetTransferCount(WS2812_DMA_CHANNEL, (uint16_t)(waveformLength - 1U));

    PIR2bits.DMA1SCNTIF = 0U;
    PIE2bits.DMA1SCNTIE = 1U;

    DMA_SelectChannel(WS2812_DMA_CHANNEL);
    DMAnCON0bits.EN = 1U;
    DMAnCON0bits.DGO = 1U;

    switch (strip->pwmModule)
    {
    case WS2812_PWM_MODULE_1:
        PWM1CONbits.EN = 1U;
        break;
    case WS2812_PWM_MODULE_2:
        PWM2CONbits.EN = 1U;
        break;
    case WS2812_PWM_MODULE_3:
        PWM3CONbits.EN = 1U;
        break;
    default:
        PIE2bits.DMA1SCNTIE = 0U;
        PIR2bits.DMA1SCNTIF = 0U;
        strip->busy = false;
        s_activeStrip[strip->pwmModule] = (WS2812_Strip_t *)0;
        return WS2812_INVALID_PARAM;
    }

    return WS2812_OK;
}

/// @brief DMA1 SCNT ISR for WS2812 PWM module 1.
/// This ISR is triggered when the DMA transfer for WS2812 PWM module 1 is complete.
/// It calls the WS2812_OnDmaTransferCompleteISR function with the appropriate PWM module.
/// @param  None
/// @return None
void __interrupt(irq(IRQ_DMA1SCNT), low_priority) WS2812_DMA1SCNT_ISR(void)
{
    uint8_t pwmModule = WS2812_PWM_MODULE_1;
    if (PIR2bits.DMA1SCNTIF == 0U)
    {
        return;
    }

    PIR2bits.DMA1SCNTIF = 0U;

    if (s_activeStrip[pwmModule] != NULL)
    {
        finalizePWMTransfer(s_activeStrip[pwmModule], getPWMDutyRegister(pwmModule));
    }
}

/// @brief Clears all LEDs to a color of (0, 0, 0) and updates the strip.
/// @param strip  The WS2812 strip structure to clear.
/// @return  WS2812_Status_t indicating success or failure of the operation.
WS2812_Status_t WS2812_Clear(WS2812_Strip_t *strip)
{
    if (strip == NULL || !strip->initialized)
    {
        return WS2812_NOT_INITIALIZED;
    }

    for (uint16_t i = 0U; i < strip->numLEDs; i++)
    {
        strip->colors[i] = (WS2812_Color_t){0U, 0U, 0U};
    }

    return WS2812_Update(strip);
}

/// @brief  Checks if the WS2812 strip is currently busy updating.
/// @param strip  The WS2812 strip structure to check.
/// @return  WS2812_Status_t indicating if the strip is busy or not.
WS2812_Status_t WS2812_isBusy(WS2812_Strip_t *strip)
{
    if (strip == NULL || !strip->initialized)
    {
        return WS2812_NOT_INITIALIZED;
    }

    if (strip->busy || DMA_IsTransferInProgress(WS2812_DMA_CHANNEL))
    {
        return WS2812_BUSY;
    }

    return WS2812_OK;
}
