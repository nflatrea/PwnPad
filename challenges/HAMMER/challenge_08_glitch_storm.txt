==============================================================================
CHALLENGE 8: GLITCH STORM
==============================================================================

CATEGORY
       Fault Injection / Voltage Glitch

SELECT
       SW1 SW2 SW3 SW4 SW5  =  0 0 0 1 0

DESCRIPTION	
       Same trick, same goal. You have to trigger a voltage glitch to enter a 
       hidden code path and retrieve the flag. But HAMMER learned. Someone
       moved the trigger signal from SDA, and the obvious pin no longer tells you
       when the dangerous code is running. You must discover the new trigger 
       pin and timing to successfully glitch the device.

OBJECTIVES
       1. Analyze the board to find the new trigger signal.
       2. Time and inject the glitch precisely.
       3. Reach the hidden code and read the flag over UART.
