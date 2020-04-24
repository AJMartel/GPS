# GPS Speedometer with MAX and AVG speed

by morganlowe April 21, 2020

https://www.thingiverse.com/thing:4303760

Buy one at https://www.etsy.com/listing/798709535/gps-speedometer-with-max-and-avg-speed

The code has been updated and lightly optimized.

gpsspeed-lion.ino
- Settings are hard coded and set before compiling on lines 14 and 15

GPS_Lion.ino
- with additional hardware, settings can be made by end user an adjusted before power on.

   GND --- 1K Resistor ---   Element_Pin(A1)   --- SWITCH_1 --- |_+5V from Pulse_Pin(A0)
   
   GND --- 1K Resistor --- Measurement_Pin(A2) --- SWITCH_2 --- |
   
- By detecting settings in on boot "void setup(){DETECT-HERE}", 

  it saves battery power by only using current when it checks for settings.
  
- No jumpers defaults to LAND, IMPERIAL (MPH, feet)

- Element_Pin(A1) sets LAND = 0, SEA = 1

- Measurement_Pin(A2) sets IMPERIAL = 0, METRIC = 1
