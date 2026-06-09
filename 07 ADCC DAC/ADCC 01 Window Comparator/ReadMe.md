# ADCC 01 Window Comparator Hysteresis

## Overview
Use ADCC thresholding with hysteresis to report only meaningful analog changes.

The ADCC is configured as a window comparator with hysteresis to detect analog
state changes while rejecting edge chatter. This example uses a 1 V to 3 V
window. A 10k Ohm potentiometer is connected between +5 V and GND, and the
wiper drives the analog input.

When the input is within the window, the firmware reports "Inside Window" and
turns on an LED. When the input is outside the window, it reports
"Outside Window" and turns the LED off.

Hysteresis prevents rapid toggling when the input is near the upper or lower
threshold.