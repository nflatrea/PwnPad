       ___         ___         ___         ___         ___         ___
      /  /\       /  /\       /  /\       /  /\       /  /\       /  /\
     /  /:/      /  /::\     /  /::|     /  /::|     /  /::\     /  /::\
    /  /:/      /  /:/\:\   /  /:|:|    /  /:|:|    /  /:/\:\   /  /:/\:\
   /  /::\ ___ /  /::\ \:\ /  /:/|:|__ /  /:/|:|__ /  /::\ \:\ /  /::\ \:\
  /__/:/\:\  //__/:/\:\_\:/__/:/_|::::/__/:/_|::::/__/:/\:\ \:/__/:/\:\_\:\
  \__\/  \:\/:\__\/  \:\/:\__\/  /~~/:\__\/  /~~/:\  \:\ \:\_\\__\/~|::\/:/
       \__\::/     \__\::/      /  /:/      /  /:/ \  \:\ \:\    |  |:|::/
       /  /:/      /  /:/      /  /:/      /  /:/   \  \:\_\/    |  |:|\/
      /__/:/      /__/:/      /__/:/      /__/:/     \  \:\      |__|:|~
      \_H\/       \_A\/       \_M\/       \_M\/       \_E\/       \_R\|

   B  R  E  A  K    I  T    U  N  T  I  L    Y  O  U    M  A  K  E    I  T

HAMMER(7)               HAMMER Industries Field Manual               HAMMER(7)

NAME
       hammer : A shady board recovered from a HAMMER Inc. trashcan

SYNOPSIS
       While browsing behind a closed HAMMER Inc. plant, you spot a
       battered blue board in a trashcan. No documentation, no schematic, no
       source. Just five DIP switches, three LEDs, four buttons and a pile
       of pin headers.

       You know that big industries does not throw away working hardware by
       accident. Somebody left this board with secrets still inside. Find them.


HOW TO READ THIS PAGE
       Every challenge has the same entry (THE FIND) and the same goal:
       recover a flag shaped like TS{...}. Each one lists its SELECT
       switch positions, a DESCRIPTION, and OBJECTIVES.

       HINT: Most challenges end in a UART console. Type HELP to see what it
       offers.

==============================================================================
CHALLENGE 1: SERIAL SNITCH
==============================================================================

CATEGORY
       UART
      
SELECT
       SW1 SW2 SW3 SW4 SW5  =  1 0 0 0 0

DESCRIPTION
       The deice is supposed to be controlled somewhere. You just need to find 
       out how, and learn its language.

OBJECTIVES
       1. Identify the UART pins on the board.
       2. Determine the correct baud rate.
       3. Open the console and take control of the lighting.

SPOILER
       FLAG    TS{U@rt_15_@w3s0m3}
       HOW     9600 baud, 8N1. Type FLAG. (LED 1 H switches an LED on.)

==============================================================================
CHALLENGE 2: ECHO CHAMBER
==============================================================================

CATEGORY
       UART / Logic Analysis
       
SELECT
       SW1 SW2 SW3 SW4 SW5  =  0 1 0 0 0

DESCRIPTION
       Same pinout, same behaviour, except your terminal now prints garbage.
       Somebody at HAMMER decided that "standard" was for amateurs.
       
OBJECTIVES
       1. Identify the UART pins.
       2. Work out the non-standard baud rate by measuring the signal.
       3. Open a reliable terminal session and take the flag.

SPOILER
       FLAG    TS{N0w_Y0u_@r3_@n_el173_H@(k3r}
       HOW     Shortest bit about 32 us, so roughly 31250 baud (the board
               was set to 31337; the hardware really runs 31250). Either
               value works. Type FLAG.


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

SPOILER
       FLAG    TS{(h@||eng3_07P_i5_<10 digits>}
               The 10 digits are random on every boot
       HOW     Nothing ever ACKs. The message IS the traffic: each byte of
               the flag is sent as an I2C address. Decode every address
               byte (shift right by one) as ASCII. The sequence repeats
               every second.


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

SPOILER
       FLAG    TS{1_N33D_/\/\y_8u66y}
       HOW     The board polls address 0x50. Build a slave at 0x50 that
               ACKs. Once something ACKs, the board writes register 0x01
               plus 4 random token bytes (new on every boot), then reads 4
               bytes back. Store what arrives in onReceive, return it in
               onRequest. On success the UART console switches to admin
               (STATUS, WHOAMI), then type FLAG. The token is not in the
               firmware, so dumping the flash will not help.


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

SPOILER
       FLAG    TS{F1rmw@re_S3cret}
       HOW     Dump the flash (avrdude, -p m328p, flash read). The code is
               plain text in the image, a Morse string:
               .... ....- -- -- ...-- .-.   (H4MM3R)
               Enter it with DOT, DASH and SPACE (between letters), then
               OK. The console on UART 9600 echoes each key. Type FLAG.

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

SPOILER
       FLAG    TS{EEPR0M_L34ks_53cr3ts} (stored in the internal EEPROM)
       HOW     In this position the firmware does nothing. Read the EEPROM
               directly over ISP (avrdude, eeprom read) and look at the
               bytes.


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

SPOILER
       FLAG    TS{Gl1th3s_@r3_Awe50m3_!}
       HOW     SDA (A4) goes high while a 200 x 200 counting loop runs and
               goes low when it ends. Glitch inside the high window. When
               the counters come out wrong, the board prints them and
               opens a console. Type FLAG.


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

SPOILER
       FLAG    TS{0h_n0_y0u_foun6_my_7r1gg3r}
       HOW     The trigger is now one of the LED lines (D3, the middle
               LED). It goes high while the loop runs. Watch the LED pins
               on a logic analyzer, glitch inside that window, then type
               FLAG on the console (9600 baud).


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

SPOILER
       FLAG    TS{Y0u_3nter36_th3_v@ul7}
       HOW     Digits are DOT=1, DASH=2, SPACE=3, five long, OK to submit.
               The check stops at the first wrong digit and spends 10 ms
               on every correct one first. A wrong PIN then drops all LEDs
               for 500 ms, so measure OK-to-LEDs-off. At most 15 tries.
               Success blinks the LEDs and opens the console. Type FLAG.


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

SPOILER
       FLAG    TS{D@mn_y0u_@r3_g006}
       HOW     Digits are DOT=1, DASH=2, SPACE=3, OK=4, and the PIN is
               random on every boot. The check runs about 300 ms after the
               fourth press and costs 10 ms per correct leading digit
               before the 500 ms LED blackout. At most 16 tries. Type FLAG.


==============================================================================
CHALLENGE 11: CHAOS CHAIN, GLITCHGATE
==============================================================================

CATEGORY
       UART / Fault Injection (chained)

SELECT
       SW1 SW2 SW3 SW4 SW5  =  1 1 0 1 0

DESCRIPTION
       No schematic, no source, no hints. Just the bare device, a login
       screen behind a strange baud rate and your tools. Break the chain
       link by link.

OBJECTIVES
       1. Identify the strange UART baud rate.
       2. Bypass the login screen with a fault-injection attack.
       3. Take over the device and read the flag over the console.

SPOILER
       FLAG    TS{L0gin_Succ355fu1_!}
       HOW     The line really runs at about 57143 baud (the firmware asks
               for 57634). 57600 works in a terminal. The password is
               d3@1bt7*0PqSxc9~^ and is not stored in the flash. Send any
               wrong password and glitch the compare so the failure branch
               is skipped. "Login OK" opens the console. Type FLAG.


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

SPOILER
       FLAG    TS{D0n7_M355_w17h_t1m3}
       HOW     Software UART, 1200 baud: RX on D12, TX on D11. The PIN is
               25315294 and does not change between boots. The check stops
               at the first wrong digit and spends 10 ms on every correct
               one, so time how long the board takes to answer after you
               send the newline (about 80 tries). The correct PIN opens a
               console; type FLAG.


==============================================================================
CHALLENGE 13: PIZZA ORDER
==============================================================================

CATEGORY
       Side-Channel / Power Analysis (SPA)

SELECT
       SW1 SW2 SW3 SW4 SW5  =  1 0 1 1 0

DESCRIPTION

       The device asks for a password. You've heard that it apparently leaks 
       through its power consumption. A Simple Power Analysis (SPA) with a basic
       handmade oscilloscope is sufficient, but you must choose the right probe
       point and capture parameters.

       The password can be exfiltrated by observing and interpreting power 
       waveforms during the authentication routine (no invasive decapping or 
       destructive faults required).
	
	HINT: If you have a real working oscilloscope, use it. If not, you might
	need to build one with what you have.
	
OBJECTIVES
       1. Pick a safe power probe point (VCC or a decoupling capacitor)
          and connect without shorting anything.
       2. Trigger the login while recording the power draw.
       3. Align and compare traces for repeatable features.
       4. Reconstruct the password, log in, and take the flag.

SPOILER
       FLAG    TS{5P@_ar3_H4rd_Bu7_r3w0rd1n9}
       HOW     Console at 9600 baud. SDA (A4) is high while the board
               waits for the password and falls the moment Enter arrives,
               and the compare runs right after that falling edge. Trigger
               on it. The password is hacktheplanet. Type FLAG.

