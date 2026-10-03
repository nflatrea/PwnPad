==============================================================================
CHALLENGE 6: HARD LEAK
==============================================================================

CATEGORY
       EEPROM Extraction

SELECT
       SW1 SW2 SW3 SW4 SW5  =  0 1 1 0 0

DESCRIPTION
       Damn it! You already tore the firmware apart and found nothing else 
       useful. The chip still has one more place to hide things, a small EEPROM
       that keeps its contents when powered off. Nobody ever remembers to wipe 
       it tho.

OBJECTIVES
       1. Work out how to reach the chip's internal EEPROM over ISP.
       2. Read it out.
       3. Recover the flag from the leaked data.
