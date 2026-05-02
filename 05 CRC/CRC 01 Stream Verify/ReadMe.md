# CRC 01 Stream Verify

## Overview
Planned demonstration for CRC behavior on PIC18F Q43 devices.  This project uses the same 
schematic as the UART 03 DMA Rx project.  This uses hardware flow control to prevent
buffer overrun of the receive buffer.


## Expected outcome
The project should echo back all data sent to it up to a CR+LF (or LF+CR) sequence as 
the input data, and a computed CRC value for that string, as well as if it matches the 
expected CRC or not. 

If you enter the text "This is a demonstration" + CR + LF, the expected CRC should match.
Any other input should not match.