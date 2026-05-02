/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for SPI 01 Host Loopback Transfer.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "spi.h"

void main(void)
{
    SYSTEM_Initialize();
    SPI1_Initialize();

    while (1)
    {
        SPI1_Task();
    }
}
