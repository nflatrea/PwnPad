==============================================================================
CHALLENGE 9: CLOCK SPY
==============================================================================

CATEGORY
       Side-Channel / Timing Attack

SELECT
       SW1 SW2 SW3 SW4 SW5  =  1 0 0 1 0

DESCRIPTION
       A 5-digit PIN guards this device, entered on DOT/DASH/SPACE and
       submitted with OK. The PIN is random on every boot, so you must recover
       it live. The check bails out at the first wrong digit, which means
       correct digits take measurably longer to reject than wrong ones. Time
       each guess and work the PIN one digit at a time.
  
DISCLAIMER
       Normally, side-channel attacks require expensive equipment such as 
       oscilloscopes and specialized tooling like a ChipWhisperer. However, we 
       have intentionally added specific delays so these attacks can be 
       performed using a very affordable logic analyzer.

OBJECTIVES
       1. Watch the timing of the PIN check.
       2. Use that to recover the digits one at a time.
       3. Open the vault and read the flag over UART (9600 baud).
