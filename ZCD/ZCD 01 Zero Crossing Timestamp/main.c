/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for ZCD 01 Zero Crossing Timestamp.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "zcd.h"

void main(void)
{
    SYSTEM_Initialize();
    ZCD1_Initialize();

    while (1)
    {
        ZCD1_Task();
    }
}
