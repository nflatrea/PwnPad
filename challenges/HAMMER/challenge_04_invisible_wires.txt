==============================================================================
CHALLENGE 4: INVISIBLE WIRES
==============================================================================

CATEGORY
       I2C / Device Spoofing

SELECT
       SW1 SW2 SW3 SW4 SW5  =  0 0 1 0 0

DESCRIPTION

       After ripping the main board from its casing and making your escape,
       a sudden realization hits (you left another board behind), still 
       wired in and powered.

       Unexpectedly, the stolen board is trying to communicate with its twin 
       over I2C, as if expecting a response. Spoof the board and be the twin.

OBJECTIVES
       1. Identify the I2C lines and the address being polled.
       2. Emulate an I2C slave at that address.
       3. Capture and replay what the board expects, then retrieve the flag
          from the UART console (9600 baud).
