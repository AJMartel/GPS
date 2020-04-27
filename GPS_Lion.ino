/* GPS Speedo by Morgan Lowe. https://www.thingiverse.com/thing:4303760 
   I dont recall were I found the custom font so uh, whoever made, go you!
   Big Font very similar to https://forum.arduino.cc/index.php?topic=8882.0
   uses Arduino Nano and I2C 20x4 LCD with Serial GPS
   Uses Statistics for arduino by  robtillaart  http://playground.arduino.cc/Main/Statistics
   Uses TinyGPS++ by Arduiniana http://arduiniana.org/libraries/tinygpsplus/
   Do what you will with this!
   On the nano SDA is pin A4 and SCL is pin A5
   Rx from GPS to pin 4 and TX to pin 3.
   Wire Pixels to pin 7.
   Configure Units before powering on the device
   GND --- 1K Resistor ---   Element_Pin(A1)   --- SWITCH_1 --- |___+5V from Pulse_Pin(A0)
   GND --- 1K Resistor --- Measurement_Pin(A2) --- SWITCH_2 --- |
   Have fun! 
   Thanks to AJMartel https://github.com/AJMartel/GPS for the improved code!
*/
#include <avr/pgmspace.h>              //Used to store "constant variables" in program space (mainly for messages displayed on LCD)
#include <Wire.h>                      // Comes with Arduino IDE++
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <TimeLib.h>
#include "Statistic.h"
#include <Adafruit_NeoPixel.h>

/*  NeoPixel LED Driver
 Which pin on the Arduino is connected to the NeoPixels?
 How many NeoPixels are attached to the Arduino?
 set number of pixels here, I have 4 in a strip with the first disabled because it's under the heat shrink.
 When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
 Note that for older NeoPixel strips you might need to change the third parameter
 --see the strandtest example for more information on possible values.
*/
#define NEO_PIN            4               
#define NUMPIXELS          4
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

// The serial connection to the GPS device
#define RXPin              2
#define TXPin              3
SoftwareSerial ss(RXPin, TXPin);
// The TinyGPS++ object
TinyGPSPlus gps;
TinyGPSCustom zdop(gps, "GPVTG", 7);   // $GPVTG sentence, 7th element
static const uint32_t GPSBaud = 9600;  //GPS Baud rate, set to default of Ublox module
Statistic GPSStats;                    //setup stats

//NOTE: CHANGE A6 & A7 if not using NANO (Not available on UNO)
#define Element_PIN        A2          // LAND or SEA        >If SEA the unit will be in Knots, LAND could be MPH or KPH
#define Measurement_PIN    A1          // IMPERIAL or METRIC or  >If IMPERIAL MPH and Feet, METRIC KPH and Meters       
#define testPIN            A0          // Used to Trigger Element_PIN and Measurement_PIN 
#define UTC_offsetPIN0     5           //Used to set offset 1s bit
#define UTC_offsetPIN1     6           //Used to set offset 2s bit
#define UTC_offsetPIN2     7           //Used to set offset 4s bit
#define UTC_offsetPIN3     8           //Used to set offset 8s bit
#define UTC_offsetPIN4     9           //Used to set offset by half an hour
#define UTC_offsetPIN5     10          //Used to set offset +/- bit

/* Hours
 * Pin    10 9 8 7 6 5 Value
 * Switch  6 5 4 3 2 1 
 *         0 0 0 0 0 0   0
 *         0 0 0 0 0 1   1
 *         0 0 0 0 1 0   2
 *         0 0 0 0 1 1   3 
 *         0 0 0 1 0 0   4
 *         0 0 0 1 0 1   5
 *         0 0 0 1 1 0   6
 *         0 0 0 1 1 1   7
 *         0 0 1 0 0 0   8
 *         0 0 1 0 0 1   9
 *         0 0 1 0 1 0   10
 *         0 0 1 0 1 1   11
 *         0 0 1 1 0 0   12
 * Modifiers
 * Pin    10 9 8 7 6 5 Value
 * Switch  6 5 4 3 2 1
 *         0 0 0 0 0 0   +0.0 
 *         0 1 0 0 0 0   +0.5
 *         0 0 0 0 0 0   +
 *         0 0 0 0 0 0   -
 */
              
bool ELEMENT     = 0;                  // Pin(A2) Switch(7) LAND = 0, SEA = 1
bool MEASUREMENT = 0;                  // Pin(A1) Switch(8) IMPERIAL = 0, METRIC = 1
// Offset hours from gps time (UTC)
float UTC_offset = 0;                  // Default ZULU
int localTimeOffset = 0;

//Time
uint8_t Hour     = 0;
uint8_t Minute   = 0;
uint8_t Second   = 0;
uint8_t Day      = 0;
uint8_t Month    = 0;
uint16_t Year    = 0;

LiquidCrystal_I2C lcd(0x27, 20, 4);    // Set the LCD I2C address

//Below are the custom characters for the large speed display
int x = 0;
// the 8 arrays that form each segment of the custom numbers
byte LT[8] = {
  B00111,
  B01111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte UB[8] = {
  B11111,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

byte RT[8] = {
  B11100,
  B11110,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte LL[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B01111,
  B00111
};

byte LB[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111
};

byte LR[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11110,
  B11100
};

byte UMB[8] = {
  B11111,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111
};

byte LMB[8] = {
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111
};

int onval  = 1;
int satval = 0;
int dispt  = 0;
unsigned long timeOut;                 // Backlight timeout variable
unsigned long currentMillis  = 0;
unsigned long previousMillis = 0;      // last time update
unsigned long interval       = 15000;  // interval at which to do something (milliseconds)

void setup() {
  pinMode(testPIN, OUTPUT);            //Used as a Digital HIGH Signal
  pinMode(Element_PIN, INPUT);         //Detect LAND or SEA
  pinMode(Measurement_PIN, INPUT);     //Detect IMPERIAL or METRIC
  pinMode(UTC_offsetPIN0, INPUT);      //Used to set offset 1s bit
  pinMode(UTC_offsetPIN1, INPUT);      //Used to set offset 2s bit
  pinMode(UTC_offsetPIN2, INPUT);      //Used to set offset 4s bit
  pinMode(UTC_offsetPIN3, INPUT);      //Used to set offset 8s bit
  pinMode(UTC_offsetPIN4, INPUT);      //Used to set offset by half an hour
  pinMode(UTC_offsetPIN5, INPUT);      //Used to set offset +/- bit
  
  setConfig();                         //Checks Configuration
  
  ss.begin(GPSBaud);                   //Start GPS Module
  lcd.init();                          //Initialize the lcd
  delay(10);
  
  pixels.begin();                      // This initializes the NeoPixel library.                      
  lcd.createChar(1, UB);               // assignes each segment a write number
  lcd.createChar(2, RT);
  lcd.createChar(3, LL);
  lcd.createChar(4, LB);
  lcd.createChar(5, LR);
  lcd.createChar(6, UMB);
  lcd.createChar(7, LMB);
  lcd.createChar(8, LT);
  lcd.backlight();                     // turn on backlight
  lcd.clear();                         // clear the screen - intro below!
  lcd.setCursor(1, 0);                 // put cursor at column x and row y
  lcd.print(F("   MORNINGLION "));     // print a text
  lcd.setCursor(0, 1);                 // put cursor at column x and row y
  lcd.print(F("     INDUSTRIES"));     // print a text
  lcd.setCursor(1, 2);                 // put cursor at column x and row y
  lcd.print(F("GPS data - Lion 4.0")); // print a text
  lcd.setCursor(0, 3);                 // put cursor at column x and row y
  lcd.print(F("  GPS Speedometer "));  // print a text
  GPSStats.clear();                    //Reset the stats
  delay (5000);
  lcd.clear();
}

void loop() {
  int speed;
  int satval = gps.satellites.value(); //check sats availible and set to satval
  if (satval >= 13) {
    satval = 12;
  }
  //below turns off the backlight if it's just waiting for satelites
  if (satval >= 1) {
    timeOut = millis();
    lcd.backlight();                   // turn on backlight
    onval = 1;
  }
  else if ((millis() - timeOut) > 30000) {  // Turn off backlight for 30 seconds, else turn it off
    lcd.noBacklight();                 // turn off backlight
    onval = 0;
  }
  delay(100);
  //LEDs
  // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
  if (onval == 1) {
    switch (satval) {
      case 0:
        pixels.setPixelColor(0, pixels.Color(80, 0, 0));
        pixels.setPixelColor(1, pixels.Color(80, 0, 0));
        pixels.setPixelColor(2, pixels.Color(80, 0, 0));
        break;
      case 1:
        pixels.setPixelColor(0, pixels.Color(80, 30, 0));
        pixels.setPixelColor(1, pixels.Color(80, 0, 0));
        pixels.setPixelColor(2, pixels.Color(80, 0, 0));
        break;
      case 2:
        pixels.setPixelColor(0, pixels.Color(80, 30, 0));
        pixels.setPixelColor(1, pixels.Color(80, 30, 0));
        pixels.setPixelColor(2, pixels.Color(80, 0, 0));
        break;
      case 3:
        pixels.setPixelColor(0, pixels.Color(80, 30, 0));
        pixels.setPixelColor(1, pixels.Color(80, 30, 0));
        pixels.setPixelColor(2, pixels.Color(80, 30, 0));
        break;
      case 4:
        pixels.setPixelColor(0, pixels.Color(0, 80, 0));
        pixels.setPixelColor(1, pixels.Color(80, 30, 0));
        pixels.setPixelColor(2, pixels.Color(80, 30, 0));
        break;
      case 5:
        pixels.setPixelColor(0, pixels.Color(0, 80, 0));
        pixels.setPixelColor(1, pixels.Color(0, 80, 0));
        pixels.setPixelColor(2, pixels.Color(80, 30, 0));
        break;
      case 6:
        pixels.setPixelColor(0, pixels.Color(0, 80, 0));
        pixels.setPixelColor(1, pixels.Color(0, 80, 0));
        pixels.setPixelColor(2, pixels.Color(0, 80, 0));
        break;
      case 7:
        pixels.setPixelColor(0, pixels.Color(0, 0, 80));
        pixels.setPixelColor(1, pixels.Color(0, 80, 0));
        pixels.setPixelColor(2, pixels.Color(0, 80, 0));
        break;
      case 8:
        pixels.setPixelColor(0, pixels.Color(0, 0, 80));
        pixels.setPixelColor(1, pixels.Color(0, 0, 80));
        pixels.setPixelColor(2, pixels.Color(0, 80, 0));
        break;
      case 9:
        pixels.setPixelColor(0, pixels.Color(0, 0, 80));
        pixels.setPixelColor(1, pixels.Color(0, 0, 80));
        pixels.setPixelColor(2, pixels.Color(0, 0, 80));
        break;
      case 10:
        pixels.setPixelColor(0, pixels.Color(80, 0, 80));
        pixels.setPixelColor(1, pixels.Color(0, 0, 80));
        pixels.setPixelColor(2, pixels.Color(0, 0, 80));
        break;
      case 11:
        pixels.setPixelColor(0, pixels.Color(80, 0, 80));
        pixels.setPixelColor(1, pixels.Color(80, 0, 80));
        pixels.setPixelColor(2, pixels.Color(0, 0, 80));
        break;
      case 12:
        pixels.setPixelColor(0, pixels.Color(80, 0, 80));
        pixels.setPixelColor(1, pixels.Color(80, 0, 80));
        pixels.setPixelColor(2, pixels.Color(80, 0, 80));
        break;
    }
  }
  else {                               //if the backlight is off turn off pixels too
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.setPixelColor(1, pixels.Color(0, 0, 0));
    pixels.setPixelColor(2, pixels.Color(0, 0, 0));
  }
  pixels.show();                       // This sends the updated pixel color to the hardware.

  //clear out data when there are no sats to prevent false averages
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if (gps.satellites.value() == 0) {
      GPSStats.clear();
      lcd.clear();
    }
  }

  //Add data to stats & Code for unit change
  if (ELEMENT == 0) {
    if (MEASUREMENT == 1) {
      speed = (gps.speed.kmph());      //Speed in kilometers per hour (double)
    }
    else {
      speed = (gps.speed.mph());       //Speed in miles per hour (double)
    }
  }
  else {
    speed = (gps.speed.knots());       //Speed in knots (double)  
  }
    GPSStats.add(speed);               //Send speed to Stats for average and max
    screen();
    speedcalc(speed);
    lcd.setCursor(11, 3);
  if (ELEMENT == 0) {
    if (MEASUREMENT == 1) {
      lcd.print(F("KPH"));
    }
    else {
      lcd.print(F("MPH"));
    }
  }
  else {
    lcd.print(F("Kts"));
  }  
  smartDelay(1000);
}

void setConfig() {
    digitalWrite(testPIN, HIGH);
    delay(100);
    UTC_offset = digitalRead(UTC_offsetPIN0) | (digitalRead(UTC_offsetPIN1) << 1) | (digitalRead(UTC_offsetPIN2) << 2) | (digitalRead(UTC_offsetPIN3) << 3);
  if(digitalRead(UTC_offsetPIN4) == 1) {
    UTC_offset = UTC_offset + .5;      //This will add half an hour
  }
  if(digitalRead(UTC_offsetPIN5) == 1) {
    UTC_offset = UTC_offset * - 1;     //This will turn the value negative
  }
  localTimeOffset = (UTC_offset * 3600);
  ELEMENT = digitalRead(Element_PIN);          //Sets LAND or SEA
  MEASUREMENT = digitalRead(Measurement_PIN);  //Sets IMPERIAL or METRIC
  digitalWrite(testPIN, LOW);
}

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available())
      gps.encode(ss.read());
  } 
  while (millis() - start < ms);
}


static void screen() {
  //Time setup
  Hour   = gps.time.hour();            //Hour (0-23) (u8)
  Minute = gps.time.minute();          //Minute (0-59) (u8)
  Second = gps.time.second();          //Second (0-59) (u8)
  Day    = gps.date.day();             //Day (1-31) (u8)
  Month  = gps.date.month();           //Month (1-12) (u8)
  Year   = gps.date.year();            //Year (2000+) (u16)
  // Set Time from GPS data string
  setTime(Hour, Minute, Second, Day, Month, Year);
  delay(10);
  adjustTime(localTimeOffset);
  delay(10);
  // Calc current Time Zone time by offset value
  //float dispt = (hour() + UTC_offset);
  int dispt = hour(now() + localTimeOffset);

  if (dispt == 0) {
    dispt = (dispt + 12);
  }

  if (dispt > 12) {
    dispt = (dispt - 12);
    lcd.setCursor(18, 1);
    lcd.print(F("PM"));
  }
  else {
    lcd.setCursor(18, 1);
    lcd.print(F("AM"));
  }

  //put the : in the time
  lcd.setCursor(12, 1);
  if (dispt < 10) {
    lcd.print(F(" "));
  }
  lcd.print(dispt);
  lcd.print(F(":"));
  if (minute() < 10) {
    lcd.print(F("0"));
  }
  lcd.print(minute());

  //Measure altitude
  lcd.setCursor(14, 2);
  if (MEASUREMENT == 1) {
    lcd.print(gps.altitude.meters());  // Altitude in meters (double)
  }
  else {
    lcd.print(gps.altitude.feet());    // Altitude in feet (double)
  }
    lcd.setCursor(15, 3);
  if (MEASUREMENT == 1) {
    lcd.print(F("METER"));
  }
  else {
    lcd.print(F("^FEET"));
  }

  //average speed
  lcd.setCursor(0, 1);
  lcd.print(F("AVG:"));
  lcd.print(GPSStats.average(), 0);
  if (ELEMENT == 0) {
    if (MEASUREMENT == 1) {
      lcd.print(F("KPH"));
    }
    else {
      lcd.print(F("MPH"));
    }
  }
  else {
    lcd.print(F("Kts"));
  }

  //max speed
  lcd.setCursor(0, 0);                 // put cursor at colon 0 and row 0 = left/up
  lcd.print(F("MAX:"));
  lcd.print(GPSStats.maximum(), 0);
  if (ELEMENT == 0) {
    if (MEASUREMENT == 1) {
      lcd.print(F("KPH"));
    }
    else {
      lcd.print(F("MPH"));
    }
  }
  else {
    lcd.print(F("Kts"));
  }

  // Sat Numbers
  lcd.setCursor(14, 0);
  lcd.print(gps.satellites.value());
  lcd.print(F(" SATS"));
}


void custom0O() {                      // uses segments to build the number 0
  lcd.setCursor(x, 2);
  lcd.write(8);
  lcd.write(1);
  lcd.write(2);
  lcd.setCursor(x, 3);
  lcd.write(3);
  lcd.write(4);
  lcd.write(5);
}

void custom1() {
  lcd.setCursor(x, 2);
  lcd.write(1);
  lcd.write(2);
  lcd.setCursor(x, 3);
  lcd.write(4);
  lcd.write(255);
  lcd.write(4);
}

void custom2() {
  lcd.setCursor(x, 2);
  lcd.write(6);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x, 3);
  lcd.write(3);
  lcd.write(7);
  lcd.write(7);
}

void custom3() {
  lcd.setCursor(x, 2);
  lcd.write(6);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x, 3);
  lcd.write(7);
  lcd.write(7);
  lcd.write(5);
}

void custom4() {
  lcd.setCursor(x, 2);
  lcd.write(3);
  lcd.write(4);
  lcd.write(2);
  lcd.setCursor(x + 2, 3);
  lcd.write(255);
}

void custom5() {
  lcd.setCursor(x, 2);
  lcd.write(255);
  lcd.write(6);
  lcd.write(6);
  lcd.setCursor(x, 3);
  lcd.write(7);
  lcd.write(7);
  lcd.write(5);
}

void custom6() {
  lcd.setCursor(x, 2);
  lcd.write(8);
  lcd.write(6);
  lcd.write(6);
  lcd.setCursor(x, 3);
  lcd.write(3);
  lcd.write(7);
  lcd.write(5);
}

void custom7() {
  lcd.setCursor(x, 2);
  lcd.write(1);
  lcd.write(1);
  lcd.write(2);
  lcd.setCursor(x + 1, 3);
  lcd.write(8);
}

void custom8() {
  lcd.setCursor(x, 2);
  lcd.write(8);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x, 3);
  lcd.write(3);
  lcd.write(7);
  lcd.write(5);
}

void custom9() {
  lcd.setCursor(x, 2);
  lcd.write(8);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x, 3);
  lcd.write(7);
  lcd.write(7);
  lcd.write(5);
}


void tempvar(int numar) {
  switch (numar) {
    case 0:
      custom0O();
      break;
    case 1:
      custom1();
      break;
    case 2:
      custom2();
      break;
    case 3:
      custom3();
      break;
    case 4:
      custom4();
      break;
    case 5:
      custom5();
      break;
    case 6:
      custom6();
      break;
    case 7:
      custom7();
      break;
    case 8:
      custom8();
      break;
    case 9:
      custom9();
      break;
  }
}

//calculate the speed
void speedcalc(int speed) {
  lcd.setCursor(0, 2);
  lcd.print(F("           "));
  lcd.setCursor(0, 3);
  lcd.print(F("           "));
  if (speed >= 100) {
    x = 0;
    tempvar(int(speed / 100));
    speed = speed % 100;
    x = x + 4;
    tempvar(speed / 10);
    x = x + 4;
    tempvar(speed % 10);
  }
  else if (speed >= 10) {
    lcd.setCursor(0, 2);
    lcd.print(F("   "));
    lcd.setCursor(0, 3);
    lcd.print(F("   "));
    x = 4;
    tempvar(speed / 10);
    x = x + 4;
    tempvar(speed % 10);
  }
  else if (speed < 10) {
    lcd.setCursor(0, 2);
    lcd.print(F("       "));
    lcd.setCursor(0, 3);
    lcd.print(F("       "));
    x = 8;
    tempvar(speed);
  }
}
