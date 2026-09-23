# PWM 02 Fan Control

This project demonstrates how to drive a 4-wire PWM fan according to the Intel PWM Fan standard.
These fans are primarily used in computers and other equipment where the fan speed can be adjusted 
based on temperature or other needs, and is dirrectly controllable between 0% and 100% fan 
speed. 

## Behavior

This demonstration starts with a PWM duty cycle of 0% and increases by 10% with a delay of 
5 seconds between steps.  When it reaches 100%, it ramps down again by 10% until it reaches 
0% modulation, then repeats.  There is a 5 second delay between ramp up and down steps. 

## Interrupt and Event Model
- UART1 RX and TX are handled by vectored ISRs (`IRQ_U1RX`, `IRQ_U1TX`) that call UART library 
  handlers.
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
Fan set to 0% 
Fan set to 10% 
Fan set to 20% 
... 
Fan set to 100% 
Fan set to 90% 
Fan set to 80% 
... 
```

## Notes
- This project uses the shared UART library at `../../Libraries/UARTLIB`.
