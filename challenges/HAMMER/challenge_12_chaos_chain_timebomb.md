==============================================================================
CHALLENGE 12: CHAOS CHAIN, TIMEBOMB
==============================================================================

CATEGORY
       UART / Side-Channel (chained)

SELECT
       SW1 SW2 SW3 SW4 SW5  =  0 0 1 1 0

DESCRIPTION
       THE FINAL BLACKBOX. HAMMER moved the console off the normal UART
       pins and made it slow. 
       HAMMED moved the UART pins, so you must manually identify the correct 
       pins. communication runs at a non-common baud rate. After establishing 
       communication, Behind it sits a PIN check. you must perform a timing 
       side-channel attack to extract the secret.
       
       Only the most persistent and observant will succeed.


OBJECTIVES
       1. Find the relocated UART pins by probing.
       2. Work out the slow, unusual baud rate.
       3. Break the PIN with a timing side-channel attack and read the
          flag over UART.
