# DAC 01 Reference Ladder

## Overview
Step DAC through levels and verify transfer behavior with ADCC readback.

This example uses the DAC to generate a reference voltage and the ADCC to
sample that voltage. The DAC output steps up by 0.1 V per second from 0 V to
5 V, then steps back down from 5 V to 0 V at the same rate. This triangular
sweep repeats indefinitely.

After each step, the ADCC samples the DAC output and reports the measured
voltage over UART.