# PWM 02 Fan Control

This project demonstrates how to drive a 4-wire PWM fan according to the Intel PWM Fan standard.
These fans are primarily used in computers and other equipment where the fan speed can be adjusted 
based on temperature or other needs, and is dirrectly controllable between 0% and 100% fan 
speed. 

## Behavior

This demonstration starts with a PWM duty cycle of 0% and increases by 10% with a delay of 
10 seconds between steps.  When it reaches 100%, it ramps down again by 10% until it
reaches 0% modulation, then repeats.  There is a 10 second delay between ramp up and 
down steps.  After changing the FAN duty cycle, the RPM is read and displayed, allowing
2 seconds to allow the fan to settle to the chosen speed.

At each speed change, the application waits two seconds for settling, counts RC0 tach
rising edges for one second, and prints `Fan speed: <rpm> RPM` on the UART console
approximately two seconds after the change. It then waits the remaining seven seconds.
The reading is the average over the third second, not an instantaneous measurement.
Software delays include interrupt service time and depend on the internal oscillator.

Timer1 counts the tach input through `T1CKIPPS`. RPM is calculated as
`pulses * 60 / FAN_TACH_PULSES_PER_REVOLUTION`. The constant in `config.h` defaults
to two pulses per revolution; set it to the value specified by your fan manufacturer.
This gives 30 RPM resolution. No pulses produces 0 RPM (stopped fan or missing tach
signal); a counter overflow produces an error message instead of an incorrect RPM.

## Interrupt and Event Model
- UART1 RX and TX use the flat ISR by default, or vectored ISRs (`IRQ_U1RX`,
  `IRQ_U1TX`) when `USE_VECTORED_INTERRUPTS` is defined.
- Timer1 counts tach pulses in hardware without a tach interrupt.
- Ramping is handled by simply using the delay functions.  It could also be computed from a 
  thermistor or other temperature sensing arrangement. 

## Source Files 
- `main.c`: system startup, UART open/select, and logic loop.
- `config.h`: config bits and system constants.
- `config.c`: oscillator, port setup, PPS, and other configuration.
- `pwm.h`: defintions of data ares, constants, and functions related to the PWM functions
- `pwm.c`: Implementation of the PWM functions.
- 
## Test Procedure
1. Program the device with this project.
2. Connect serial terminal at 19200,8,N,1.
3. Verify startup messages:

```text
PWM 02 Fan Control 
PWM initialized successfully.
Setting duty cycle to 0% ... Fan speed: 0 RPM
Setting duty cycle to 10% ... Fan speed: <measured rpm> RPM
... 
Setting duty cycle to 100% ... Fan speed: <measured rpm> RPM
Setting duty cycle to 90% ... Fan speed: <measured rpm> RPM
... 
```

4. Verify each RPM line appears about three seconds after its duty-cycle message,
   on both ramp directions, with about ten seconds between duty changes.
5. With the fan tach disconnected, verify 0 RPM. Some fans continue rotating at 0%
   duty, so a connected fan may legitimately report nonzero RPM there.
6. For a known-input check, disconnect the fan tach and apply a logic-level 100 Hz
   square wave to RC0 with a common ground. With two pulses per revolution, expect
   approximately 3000 RPM (one count is 30 RPM). Check 50 Hz for approximately 1500 RPM.

## Notes
- This project uses the shared UART library at `../../Libraries/UARTLIB`.
- RC0 is a digital input with its weak pull-up enabled for an open-collector tach
  output. Use a tach signal/pull-up compatible with the MCU supply voltage.  It 
  is also a very good idea to provide protection on these inputs in the case the 
  fan fails in some way.  The schematic shows the use of clamp diodes to clamp the
  signal between the supply rails of the microcontroller, as well as a current 
  limiting resistor, and a diode that only allows the fan to pull the line low.
  Clamping diodes are used on the PWM output signal as well as a current limiting
  resistor in case the fan were to fail and apply +12/+24 to the PWM signal,
  or if the fan is mis-wired.