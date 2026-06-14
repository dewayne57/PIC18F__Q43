# Temp 01 Internal Sensor Trend

## Overview
Demonstrates how to sense the temperature of the Q43 die. This value can be tracked 
over time to see if the die is heating up and to trigger a thermal shutdown, derating
of external parts, changes to the ADCC or DAC references to adjust for temperature 
changes, and so forth. 

This project simply uses the on-chip thermal sensor to sense the die temperature 
of the Q43 and to track the trend of the temperature (rising or falling).  If the 
temperature is outside the low and high limits, the microcontroller enters a sleep 
state until the temperature is within limits. 
