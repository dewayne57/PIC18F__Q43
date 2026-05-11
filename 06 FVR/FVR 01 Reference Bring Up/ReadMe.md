# FVR 01 Reference Bring Up

## Overview
This project demonstrates how to initialize the internal fixed voltage reference 
(FVR) on chip for use by the DAC and ADC peripherals.


## Expected outcome
After the microcontroller has initialized, the output should alternate between 
"FVR enabled" and "FVR disabled" approximately each second.  This text is sent
to the UART data stream to be displayed on the PC, as well as being indicated 
by an LED (on = FVR Enabled). 