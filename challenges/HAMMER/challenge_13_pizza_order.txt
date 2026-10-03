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
