/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for CLC 01 Hardware Logic Mapping.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "clc.h"

void main(void)
{
    SYSTEM_Initialize();
    CLC1_Initialize();

    while (1)
    {
        CLC1_Task();
    }
}
