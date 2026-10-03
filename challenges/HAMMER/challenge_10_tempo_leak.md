==============================================================================
CHALLENGE 10: TEMPO LEAK
==============================================================================

CATEGORY
       Side-Channel / Timing Attack

SELECT
       SW1 SW2 SW3 SW4 SW5  =  0 1 0 1 0

DESCRIPTION
       Damn it... The device now uses a 4-digit PIN, no separate OK press.
       The password verification is not triggered after all four digits have 
       been entered. Your goal remains the same, a timing side-channel attack 
       to recover the PIN. You’ll need to adjust your analysis accordingly.
       
OBJECTIVES
       1. Capture timing across the whole four-digit entry.
       2. Use the variations to work out the PIN.
       3. Get past the check and read the flag at 9600 baud.
