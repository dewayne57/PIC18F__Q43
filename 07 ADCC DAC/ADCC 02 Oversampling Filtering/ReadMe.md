# ADCC 02 Oversampling Filtering

## Overview
Compare filter settings to characterize noise reduction versus response time.

This project demonstrates ADCC oversampling and filtering on a potentially noisy
analog signal. In many applications, the analog input is erratic enough that the
instantaneous reading is less useful than the average signal level. By taking
multiple samples and combining them in hardware, the ADCC can smooth the result
and reduce the effect of input noise.

The benefit of this approach is improved noise immunity and a more stable
reported value. The tradeoff is response time: larger sample counts and heavier
filtering improve stability, but they also slow the system response to real
changes at the input. Lighter filtering responds faster, but with less noise
rejection.

This project uses the same schematic as the window comparator with hysteresis
example, but without the LED indication. Instead, the firmware reports the
sampled input voltage over UART for a 0 V to 5 V input range.