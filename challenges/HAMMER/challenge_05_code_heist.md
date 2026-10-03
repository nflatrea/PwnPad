==============================================================================
CHALLENGE 5: CODE HEIST
==============================================================================

CATEGORY
       Firmware Extraction

SELECT
       SW1 SW2 SW3 SW4 SW5  =  1 0 1 0 0

DESCRIPTION
       A UART console now demands a code on the DOT/DASH/SPACE keypad before it
       opens. The rumors suggest that entering a hidden morse code will unlock 
       the board. HAMMER never wrote the code down anywhere. But the flash knows
       it, and does not keep secrets very well.

OBJECTIVES
       1. Dump the firmware through the ISP interface.
       2. Analyze the binary and recover the hardcoded code.
       3. Enter the code on the keypad and take the flag from the console.
