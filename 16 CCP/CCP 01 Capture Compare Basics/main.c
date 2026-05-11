/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for CCP 01 Capture Compare Basics.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "ccp.h"

void main(void)
{
    SYSTEM_Initialize();
    CCP1_Initialize();

    while (1)
    {
        CCP1_Task();
    }
}
