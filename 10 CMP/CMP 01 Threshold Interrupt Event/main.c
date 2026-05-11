/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for CMP 01 Threshold Interrupt Event.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "cmp.h"

void main(void)
{
    SYSTEM_Initialize();
    CMP1_Initialize();

    while (1)
    {
        CMP1_Task();
    }
}
