/* *****************************************************************************************
 *   File Name: ws2812.h
 *   Description: NeoPixel WS2812 LED strip interface for the demonstration project.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-19
 *
 *   Copyright (c) 2026, Dewayne Hafenstein.
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 ***************************************************************************************** */

#ifndef WS2812_H
#define WS2812_H
#include <stdbool.h>
#include <stdint.h>

// Define the signature for the WS2812_Strip_t structure to verify its integrity
#define WS2812_SIGNATURE 0xDEADBEAF

// Define the maximum number of LEDs supported by the WS2812 strip
#define WS2812_MAX_LEDS 100

#define WS2812_0_DUTY_CYCLE 0.3f // 30% duty cycle for logical '0'
#define WS2812_1_DUTY_CYCLE 0.7f // 70% duty cycle for logical '1'
#define WS2812_RESET_TIME_US 50  // Reset time in microseconds for the WS2812 strip

typedef enum
{
    WS2812_PWM_MODULE_1 = 0, // PWM module 1
    WS2812_PWM_MODULE_2 = 1, // PWM module 2
    WS2812_PWM_MODULE_3 = 2  // PWM module 3
} WS2812_PWM_Module_t;       // Placeholder for PWM module enumeration, to be defined as needed

// Define an enumeration for each of the possible data pins for the WS2812 strip
typedef enum
{
    WS2812_PIN_RB0 = 0,  // RB0 pin for WS2812 data
    WS2812_PIN_RB1 = 1,  // RB1 pin for WS2812 data
    WS2812_PIN_RB2 = 2,  // RB2 pin for WS2812 data
    WS2812_PIN_RB3 = 3,  // RB3 pin for WS2812 data
    WS2812_PIN_RB4 = 4,  // RB4 pin for WS2812 data
    WS2812_PIN_RB5 = 5,  // RB5 pin for WS2812 data
    WS2812_PIN_RB6 = 6,  // RB6 pin for WS2812 data
    WS2812_PIN_RB7 = 7,  // RB7 pin for WS2812 data
    WS2812_PIN_RC0 = 8,  // RC0 pin for WS2812 data
    WS2812_PIN_RC1 = 9,  // RC1 pin for WS2812 data
    WS2812_PIN_RC2 = 10, // RC2 pin for WS2812 data
    WS2812_PIN_RC3 = 11, // RC3 pin for WS2812 data
    WS2812_PIN_RC4 = 12, // RC4 pin for WS2812 data
    WS2812_PIN_RC5 = 13, // RC5 pin for WS2812 data
    WS2812_PIN_RC6 = 14, // RC6 pin for WS2812 data
    WS2812_PIN_RC7 = 15  // RC7 pin for WS2812 data
} WS2812_DATA_PIN;

// Define a structure to hold the color values for each LED in the WS2812 strip
typedef struct
{
    uint8_t red;   // Red color value (0-255)
    uint8_t green; // Green color value (0-255)
    uint8_t blue;  // Blue color value (0-255)
} WS2812_Color_t;

// Define a structure to hold the configuration and state of the WS2812 strip
typedef struct
{
    uint32_t signature;                     // Signature to verify the structure integrity
    bool initialized;                       // Flag to indicate if the strip has been initialized
    bool busy;                              // Flag to indicate if the strip is currently updating
    uint16_t numLEDs;                       // Number of LEDs in the strip
    WS2812_PWM_Module_t pwmModule;          // PWM module used for data transmission
    WS2812_DATA_PIN dataPin;                // Data pin for the WS2812 strip
    WS2812_Color_t colors[WS2812_MAX_LEDS]; // An array of color values for each LED
} WS2812_Strip_t;

/// @brief Enumeration for WS2812 status codes.
typedef enum
{
    WS2812_OK = 0,               // Operation successful
    WS2812_ERROR = -1,           // General error
    WS2812_INVALID_PARAM = -2,   // Invalid parameter passed to function
    WS2812_NOT_INITIALIZED = -3, // Strip not initialized
    WS2812_OUT_OF_BOUNDS = -4,   // LED index out of bounds
    WS2812_BUSY = -5             // Strip is busy updating
} WS2812_Status_t;

/// @brief Function to initialize the WS2812 strip with the specified data pin and number of LEDs.
/// @param strip The WS2812 strip structure to initialize.
/// @param module The PWM module to use for data transmission.
/// @param dataPin  The data pin to use for the WS2812 strip.
/// @param numLEDs  The number of LEDs in the strip.
/// @return WS2812_Status_t indicating success or failure of the operation.
WS2812_Status_t WS2812_Init(WS2812_Strip_t *strip, WS2812_PWM_Module_t module, 
    WS2812_DATA_PIN dataPin, uint16_t numLEDs);

/// @brief Sets the color of the specified LED in the WS2812 strip.
/// @param strip  The WS2812 strip structure.
/// @param index  The index of the LED to set the color for (0-based).
/// @param color  The color to set for the specified LED.
/// @return  WS2812_Status_t indicating success or failure of the operation.
WS2812_Status_t WS2812_SetColor(WS2812_Strip_t *strip, uint16_t index, WS2812_Color_t color);

/// @brief Updates the WS2812 strip with the current color values for each LED.
/// @param strip  The WS2812 strip structure to update.
/// @return  WS2812_Status_t indicating success or failure of the operation.
WS2812_Status_t WS2812_Update(WS2812_Strip_t *strip);

/// @brief Services a pending WS2812 DMA transfer and clears the busy state when complete.
/// @note Call this periodically from the main loop while a transfer is active.
/// @param strip  The WS2812 strip structure to service.
/// @return  WS2812_Status_t indicating whether the strip is busy or idle.
WS2812_Status_t WS2812_Service(WS2812_Strip_t *strip);

/// @brief Clears all LEDs to a color of (0, 0, 0) and updates the strip.
/// @param strip  The WS2812 strip structure to clear.
/// @return  WS2812_Status_t indicating success or failure of the operation.
WS2812_Status_t WS2812_Clear(WS2812_Strip_t *strip);

/// @brief  Checks if the WS2812 strip is currently busy updating.
/// @param strip  The WS2812 strip structure to check.
/// @return  WS2812_Status_t indicating if the strip is busy or not.
WS2812_Status_t WS2812_isBusy(WS2812_Strip_t *strip);

/// @brief ISR completion hook for WS2812 DMA source-count completion.
/// @note Call this from the DMA1SCNT interrupt handler.
void WS2812_OnDmaTransferCompleteISR(void);

#endif // WS2812_H