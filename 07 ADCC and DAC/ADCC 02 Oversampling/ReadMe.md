# ADCC 02 Oversampling Filtering

## Overview

Use ADCC hardware oversampling and averaging to improve measurement stability.

This project demonstrates ADCC oversampling and hardware averaging on a potentially noisy
analog signal. In many applications, the analog input is erratic enough that the
instantaneous reading is less useful than the average signal level. By taking
multiple samples and combining them in hardware, the ADCC can smooth the result
and reduce the effect of input noise.

The benefit of this approach is improved noise immunity and a more stable
reported value. The tradeoff is response time: larger sample counts improve
stability, but they also slow the system response to real changes at the input.

This project uses the same schematic as the window comparator example, but
without the LED indication. RA0 is the sampled analog input. The firmware
reports that input voltage over UART for a 0 V to 5 V range.

The ADCC conversion sequence is started in software about every 1 second.
The main loop does not wait for conversion completion. When the hardware
average result is ready, the ADCC interrupt copies the result and signals
the main loop to print the new value.
