/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System initialization for DAC 02 Software Servo.
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

#include <xc.h>
#include "config.h"

void SYSTEM_Initialize(void)
{
    INTCON0bits.GIEH = 0;
    INTCON0bits.GIEL = 0;

    // AN0 is the potentiometer setpoint input, AN1 is DAC output/readback.
    ANSELA = 0xFF;
    TRISAbits.TRISA0 = 1;
    TRISAbits.TRISA1 = 1;
    ANSELB = 0x00;
    ANSELC = 0x00;
    ANSELD = 0x00;

    // Configure DAC1 to generate output on AN1 using VDD/VSS references.
    PMD3bits.DAC1MD = 0;
    DAC1CON = 0;
    DAC1CONbits.NSS = 0b00;
    DAC1CONbits.PSS = 0b00;
    DAC1CONbits.OE = DAC1_OUTPUT_TO_AN1;
    DAC1DATL = (uint8_t)DAC_MV_TO_COUNTS(DAC_STARTUP_MV);
    DAC1CONbits.EN = 1;

    // Configure ADCC single-shot average mode. Firmware switches AN0/AN1 each trigger.
    ADCON0bits.ADON = 0;
    ADCON0bits.CONT = 0;
    ADCON0bits.CS = 0;
    ADCON0bits.FM = 1;
    ADCON0bits.GO = 0;

    ADCON1bits.PPOL = 0;
    ADCON1bits.IPEN = 0;
    ADCON1bits.GPOL = 0;
    ADCON1bits.DSEN = 0;

    ADCON2bits.PSIS = 0;
    ADCON2bits.CRS = ADCC_OVERSAMPLE_CRS;
    ADCON2bits.MD = 0b010;

    ADCON3bits.CALC = 0b000;
    ADCON3bits.TMD = 0b010;

    ADCLK = 32;
    ADREFbits.NREF = 0;
    ADREFbits.PREF = 0b00;
    ADPCHbits.PCH = ADCC_CHANNEL_SETPOINT_AN0;

    ADPRE = 15;
    ADACQ = 15;
    ADCAP = 0;
    ADRPT = ADCC_OVERSAMPLE_COUNT - 1;
    ADCNT = 0;
    ADFLTRH = 0;
    ADFLTRL = 0;

    ADSTPT = 0;
    ADLTH = 0;
    ADUTH = 0;

    ADACT = 0;
    ADCP = 0;

    PIE1bits.ADIE = 1;
    PIE2bits.ADTIE = 0;
    PIR1bits.ADIF = 0;
    PIR2bits.ADTIF = 0;

    ADCON0bits.ADON = 1;
    ADCON0bits.GO = 0;

    INTCON0bits.GIEL = 1;
    INTCON0bits.GIEH = 1;
}
