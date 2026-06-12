# DAC 01 Reference Ladder

## Overview
Step DAC through levels and verify transfer behavior with ADCC readback.

This example uses the DAC to generate a reference voltage and the ADCC to
sample that voltage. The DAC output steps up by 0.1 V per second from 0 V to
5 V, then steps back down from 5 V to 0 V at the same rate. This triangular
sweep repeats indefinitely.

After each step, the ADCC samples the DAC output and reports the measured
voltage over UART.

The DAC output is presented on AN1 for external measurement if desired.

## How It Works
1. The main loop updates DAC1 by 100 mV every 1000 ms.
2. The setpoint ramps 0 V -> 5 V -> 0 V continuously (triangular profile).
3. After each DAC update, firmware starts one ADCC conversion group in software.
4. The main loop does not wait for conversion completion.
5. The ADCC interrupt captures the averaged result and sets a print/report flag.
6. The main loop prints setpoint and measured voltage over UART when flagged.