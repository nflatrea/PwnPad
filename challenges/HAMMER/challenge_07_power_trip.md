==============================================================================
CHALLENGE 7: POWER TRIP
==============================================================================

CATEGORY
       Fault Injection / Voltage Glitch

SELECT
       SW1 SW2 SW3 SW4 SW5  =  1 1 1 0 0

DESCRIPTION
       Hmmm, The UART console prints a banner, then ignores every command. 
       The device appears locked down tight. But somehow after your firmware
       extraction, you find a code block that is never supposed to execute, 
       likely due to built-in protection checks. A well-placed  glitch might 
       convince the chip to run it anyway. The SDA pin gives you a clean 
       trigger.

       HINT: You don't really need to reverse engineer the firmware, description
       is enough ;)
	
OBJECTIVES
       1. Find the timing window to glitch in.
       2. Inject a voltage glitch, using SDA as the trigger.
       3. Reach the hidden code and read the flag at 9600 baud.
