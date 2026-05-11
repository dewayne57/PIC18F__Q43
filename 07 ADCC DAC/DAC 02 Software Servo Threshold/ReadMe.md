# ADC DAC 02 Software Servo Threshold

## Overview
Use the DAC and ADCC together in a software servo loop to track an analog setpoint.

A servo system is a closed-loop control system that continuously measures its own
output and adjusts it to match a desired target. The key elements are a setpoint
(the desired value), feedback (a measurement of the actual output), and a control
action (an adjustment that drives the output toward the setpoint).

This example implements a simple software servo using the PIC 18F Q43 DAC and ADCC.
A 10 kOhm potentiometer connected between +5 V and GND provides the setpoint voltage
on one ADCC channel. The DAC output is looped back to a second ADCC channel to
provide feedback. The firmware reads both channels, compares the setpoint to the
feedback, and increments or decrements the DAC register to drive the DAC output
toward the potentiometer voltage.

Once the loop is tracking, the DAC output closely follows the potentiometer position
across the 0 V to 5 V range. The sampled DAC feedback voltage is reported over UART 1
so the servo behavior can be observed in a terminal.

Servo systems are used in many real-world applications where a digital controller
must govern an analog output. Common examples include positioning a valve in
response to a flow or pressure sensor, or adjusting an airfoil based on sampled
airspeed, altitude, or attitude data.