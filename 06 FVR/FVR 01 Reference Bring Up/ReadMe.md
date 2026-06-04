# FVR 01 Reference Bring Up

## Overview
This project demonstrates how to initialize the internal fixed voltage reference 
(FVR) on chip for use by the DAC and ADC peripherals.


## Expected outcome
When the demonstration runs, it will configure the FVR (Fixed Voltage Reference), 
and the ADCC (Analog to Digital Converter with Computation).  The demonstration 
is strictly to show how to use the FVR, but the ADCC is used to sample the 
internal value of the FVR for display purposes.  Note, sampling a reference 
with an ADCC that uses the same reference is inherintely unstable and inaccurate.
This code is done to show the FVR functioning, and that the values are scaling 
up and down correctly.  Do NOT refer to the values as absolute values.

It is possible to measure it more accurately if an external reference were 
applied to the ADCC.  But, I did not add that complexity to the circuit or 
the code.  You are welcome to do that as an additional FVR project if you 
want. 

When the code runs, it will output an initial header, then continuously loop
between setting the FVR to 1.024V, 2.048V, 4.096V, and disabled.  Each iteration 
has a 5 second delay before the next phase. 

```
FVR 01 Reference Bring Up                                                       
Internal FVR sampled with ADCC (VDD ref = 5000 mV)                              
Using ADPCH=0x3F for FVR internal sample                                        
```

Each iteration then displays the set FVR value and the ADCC sampled 
internal result.
```
FVR=1024 mV, ADCC=1035 mV (raw=848)                                             
FVR=2048 mV, ADCC=2068 mV (raw=1694)                                            
FVR=4096 mV, ADCC=4137 mV (raw=3388)                                            
FVR has been disabled                                                           
```
