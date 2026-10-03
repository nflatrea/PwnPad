==============================================================================
CHALLENGE 3: BUS WHISPERER
==============================================================================

CATEGORY
       I2C / Bus Sniffing

SELECT
       SW1 SW2 SW3 SW4 SW5  =  1 1 0 0 0
       
DESCRIPTION
       UART is dead. A pair of traces that never stop toggling are available. 
       The board is talking over something, once a second, and nothing seems to
       answer. The traffic is silent to the untrained eye, but with the right 
       tools, you can eavesdrop on the device's conversation.
       
OBJECTIVES
       1. Identify the I2C lines.
       2. List which devices the board is talking to.
       3. Recover the message hidden in the traffic.
