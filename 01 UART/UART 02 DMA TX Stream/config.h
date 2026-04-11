/* *****************************************************************************************
 *   File Name: config.h
 *   Description: Device config and system init declarations for UART 02 DMA TX Stream.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* PIC18F27Q43 Configuration Bit Settings */
#pragma config FEXTOSC = OFF
#pragma config RSTOSC = HFINTOSC_64MHZ
#pragma config CSWEN = OFF
#pragma config FCMEN = OFF
#pragma config MVECEN = 1
#pragma config WDTE = OFF
#pragma config MCLRE = EXTMCLR
#pragma config LVP = ON
#pragma config XINST = OFF
#pragma config PPS1WAY = OFF

#define APP_FOSC_HZ             (64000000UL)
#define APP_UART_BAUD           (115200UL)

void SYSTEM_Initialize(void);

#endif /* CONFIG_H */
