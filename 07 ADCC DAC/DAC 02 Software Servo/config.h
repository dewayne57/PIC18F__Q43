/* *****************************************************************************************
 *   File Name: config.h
 *   Description: Configuration header for DAC 02 Software Servo Threshold.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-06-14
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
#ifndef CONFIG_H
#define CONFIG_H

// PIC 18F47Q43 Configuration Bit Settings
#pragma config FEXTOSC = OFF
#pragma config RSTOSC = HFINTOSC_64MHZ
#pragma config CSWEN = OFF
#pragma config FCMEN = OFF
#pragma config PR1WAY = 0
#pragma config CLKOUTEN = 0
#pragma config BOREN = 0
#pragma config LPBOREN = OFF
#pragma config IVT1WAY = 0
#ifdef VECTORED_INTERRUPTS_ENABLED
#pragma config MVECEN = 1
#else
#pragma config MVECEN = 0
#endif
#pragma config PWRTS = 2
#pragma config MCLRE = 1
#pragma config XINST = OFF
#pragma config LVP = 1
#pragma config STVREN = ON
#pragma config PPS1WAY = 0
#pragma config ZCD = 1
#pragma config BORV = 0
#pragma config WDTE = OFF
#pragma config SAFEN = OFF
#pragma config BBEN = OFF
#pragma config WRTAPP = OFF
#pragma config WRTSAF = OFF
#pragma config WRTC = OFF
#pragma config WRTB = OFF
#pragma config WRTD = OFF
#pragma config CP = OFF

#define _XTAL_FREQ 64000000UL
#define CRLF "\r\n"

void SYSTEM_Initialize(void);

// Shared voltage/reference constants.
#define APP_VREF_MV 5000
#define ADCC_MAX_COUNTS 4095
#define DAC_MAX_COUNTS 255

// Conversions with rounding.
#define ADCC_MV_TO_COUNTS(mv) ((((unsigned long)(mv) * ADCC_MAX_COUNTS) + (APP_VREF_MV / 2)) / APP_VREF_MV)
#define ADCC_COUNTS_TO_MV(counts) ((((unsigned long)(counts) * APP_VREF_MV) + (ADCC_MAX_COUNTS / 2)) / ADCC_MAX_COUNTS)
#define DAC_MV_TO_COUNTS(mv) ((((unsigned long)(mv) * DAC_MAX_COUNTS) + (APP_VREF_MV / 2)) / APP_VREF_MV)

// ADCC averaging settings.
#define ADCC_OVERSAMPLE_COUNT 16
#define ADCC_OVERSAMPLE_CRS 4

// ADCC channels for this design.
#define ADCC_CHANNEL_SETPOINT_AN0 0b00000
#define ADCC_CHANNEL_FEEDBACK_AN1 0b00001

// Servo loop behavior.
#define SERVO_SAMPLE_PERIOD_MS 2
#define SERVO_REPORT_PERIOD_MS 100
#define SERVO_DEADBAND_COUNTS 3
#define SERVO_STEP_COUNTS 1
#define SERVO_FILTER_SHIFT 2

// DAC startup/output settings.
#define DAC_STARTUP_MV 0
#define DAC1_OUTPUT_TO_AN1 0b010

#endif /* CONFIG_H */
