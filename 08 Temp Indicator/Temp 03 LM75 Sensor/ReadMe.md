# Temp 04 LM75 Sensor

The LM75 is a fully integrated temperature sensor using a 9-bit ADC and a band-gap 
reference to provide an accurate and linear temperature measurement.  It provides 
for automatic temerature threshold alerting.  The demonstration circuit shown 
uses a single LM75 at address x'90' (a0, a1, and a2 are set to 0), but the LM75 
allows for up to 8 devices to be placed on the same bus.

This circuit simply senses the temperature of the LM75 and reports it to the 
UART (console) every 1 second.  An over temperature value is configured for 
about 86F (30C) so that touching the LM75 should heat it enough to trigger 
the over temp alarm.

The alarm condition will remain until the temperature falls below the hysteresis
set point of 80.6F (27C). 