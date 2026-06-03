# FVR 01 Reference Bring Up

## Overview
This project demonstrates how to initialize the internal fixed voltage reference 
(FVR) on chip for use by the DAC and ADC peripherals.


## Expected outcome
After the microcontroller has initialized, the output should alternate between 
1.024, 2.048, and 4.096 volts output at RA2, with appropriate messages indicating 
when each voltage is output. 