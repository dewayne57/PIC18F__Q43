# Temp 02 Thermistor Linearization 

## Overview 
Use a thermistor that reads about 10K at room temperature and has a negative temperature coefficient.  This allows 
the sensed voltage to increase as the temperature increases (the NTC thermistor decreases in value).  Calibrate the
sensor at several temperatures if possible, such as freezing (0C or 32F), room temperature (25C or 77F), and boiling
water (100C or 212F).  This 3 point calibration should provide a good linearization of the thermistor.  Alternatively,
if you have access to the linearization tables of the thermistor you are using, these can be entered directly.

Alternatively, you can also use the thermistors `beta` value and the temperature at one point (usually 25C). The 
software can correct the temperature reading, if you know the beta of the thermistor you are using.

In the example circuit I used a TDK NTCDS3HG103HC3NB. The beta for this part is 3400K, has a resistance of 10K at 25C,
and a Beta tolerance of 2%.  It is a relatively inexpensive part.  You can use any thermistor you like (including PTC) 
but the software and/or hardware will need to be adjusted appropriately.

It is also important to realize that the ADCC is operated from the FVR (Fixed Voltage Reference) set to 4.096V.
This means the full-scale reading cannot exceed 4.096V, and requires the proper scaling of the series resistor. For
the thermistor used, its resistance at 100C was 998 ohms.  To have the junction voltage <= 4.096v means the series 
resistor needs to be 4.492K ohms.  The closest 1% resistor is 4.42K ohms.

 In
the event that more than 4.096V is applied to the ADCC, it will saturate at its maximum count and we will not be 
able to sense the temperature accurately.  As long as the applied voltage is less than or equal to Vcc, no damage
will be done to the controller.

The software has different symbols that can be defined to demonstrate using the calibration points, the linearization
tables, or the beta values and tolerances.  The code must be compiled with one of these macros enabled to demonstrate
a specific approach.

Note that the software uses the internal Fixed Voltage Reference (FVR) set to 4.096 volts.  The input from the 
thermistor should be constrained to between 0 and 4V.

## Demonstration Methods

### 3-point Calibration 
To demonstrate the 3-point calibration method, first measure the resistance of your thermistor at 3 known 
temperatures (0C, 25C, and 100C).  If you measure them at different temperatures than the code, make sure
you use temperatures that are far apart and update the code for the temperatures you used.

Edit the code to change the calibration values and recorded temperatures.  The code as supplied is valid 
for the TDK NTCDS3HG103HC3NB thermistor.  These calibrated resistances are: 
- Beta: 3400 (0-85C)
- 0C:  21.18K Ohms
- 25C: 10.00K Ohms
- 100C: 0.9982K Ohms

Define the symbol `THERMISTOR_3POINT_CALIBRATION` and build the project. 

### Linearization Table

The code also shows the use of a linearization table for this same part.  If you want to generate your own 
linearization table, measure the resistance of your part at 10C intervals from 0C to 100C.  The more accurate
you can get the temperatures the better.  Note, for extreme accuracy, you can also create a linearization 
table on 1C increments but that becomes tedious at best.  In some cases, there are data sheets that provide 
the linearization tables.

An alternate approach is to generate a linearization table in software theoretically from the beta and 
known resistance calibration points.  Then measure and adjust the values for the specific part. 

Define the symbol `THERMISTOR_LINEARIZATION_TABLE` and build the project.

### Beta value, Known 25C Resistance, and Tolerance 

Another approach to linearizing thermistors is to use the resistance at a known reference temperature 
(usually 25C), the device's "beta" value, and the beta tolerance.  If you know these values for your 
specific part, you can adjust the software appropriately. 

Define the symbol `THERMISTOR_BETA_VALUE` and build the project.

## Modification Instructions 

### Using a different NTC Thermistor 
A NTC (Negative Temperature Coefficient) thermistor DECREASES in resistance as the temperatire increases.  This 
type of thermistor is usually what is found in most circuits, it is slightly cheaper, and relatively easy to 
use.  However, there are vast differences in the sensitivity and the reistance values at various temperatures 
based on the specific mix of the materials used in the thermistor.  Different compounds have different "curves" 
(resistance VS temperature) and react at different speeds or with different sensitivity. 

Therefore, picking any "junk box" thermistor, while it may work, will not be accurate unless you calibrate 
the system, obtain the Beta and Beta Tolerance for the part, or obtain or create the linearization tables. 
You can create your own linearization tables by measuring the resistance of the part at specific temperatures, 
but that usually requires an accurate thermometer and heat/chill source.

In addition, you can probably measure the resistance at several key temperatures (0C, 25C, and 100C) and 
locate a part that has similar values at these temperatures.  Use the Beta of that part, and you'll be very 
close.

### Using a PTC Thermistor
A PTC (Positive Temperature Coefficient) thermistor INCREASES in resistance with increases in temperature.  To 
use it where the voltage of the divider increases with temperature, invert the thermistor and resistor in the 
divider (put the fixed resistance "on top" and the thermistor "on the bottom"), or compute the temperature using 
a decreasing value. 
