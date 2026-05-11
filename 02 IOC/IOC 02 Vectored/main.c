/* ***************************************************************************************** 
   *   File Name: main.c
   *   Description: This file contains the main program for the IOC Single project.
   *   Author: Dewayne Hafenstein
   *   Date: 2026-04-09
   ***************************************************************************************** */

     #include <xc.h>
     #include <stdio.h>
     #include "config.h"
     #include "uart.h"

   void main(void) {
         // Initialize the system and UART debug channel.
         SYSTEM_Initialize();
         UART1_Initialize();
         printf("IOC\\r\\n");
    
         while (1) {
              // Main loop
              // Add your application code here
         }
    }
