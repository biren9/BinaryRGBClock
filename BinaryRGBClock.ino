/*
   ShiftPWM non-blocking RGB fades example, (c) Elco Jacobs, updated August 2012.

   This example for ShiftPWM shows how to control your LED's in a non-blocking way: no delay loops.
   This example receives a number from the serial port to set the fading mode. Instead you can also read buttons or sensors.
   It uses the millis() function to create fades. The block fades example might be easier to understand, so start there.

   Please go to www.elcojacobs.com/shiftpwm for documentation, fuction reference and schematics.
   If you want to use ShiftPWM with LED strips or high power LED's, visit the shop for boards.
*/

#define modeSwitch 5

// ShiftPWM uses timer1 by default. To use a different timer, before '#include <ShiftPWM.h>', add
// #define SHIFTPWM_USE_TIMER2  // for Arduino Uno and earlier (Atmega328)
// #define SHIFTPWM_USE_TIMER3  // for Arduino Micro/Leonardo (Atmega32u4)

// Clock and data pins are pins from the hardware SPI, you cannot choose them yourself.
// Data pin is MOSI (Uno and earlier: 11, Leonardo: ICSP 4, Mega: 51, Teensy 2.0: 2, Teensy 2.0++: 22)
// Clock pin is SCK (Uno and earlier: 13, Leonardo: ICSP 3, Mega: 52, Teensy 2.0: 1, Teensy 2.0++: 21)

// You can choose the latch pin yourself.
const int ShiftPWM_latchPin = 9;

// ** uncomment this part to NOT use the SPI port and change the pin numbers. This is 2.5x slower **
// #define SHIFTPWM_NOSPI
// const int ShiftPWM_dataPin = 11;
// const int ShiftPWM_clockPin = 13;


// If your LED's turn on if the pin is low, set this to true, otherwise set it to false.
const bool ShiftPWM_invertOutputs = false;

// You can enable the option below to shift the PWM phase of each shift register by 8 compared to the previous.
// This will slightly increase the interrupt load, but will prevent all PWM signals from becoming high at the same time.
// This will be a bit easier on your power supply, because the current peaks are distributed.
const bool ShiftPWM_balanceLoad = false;

#include <ShiftPWM.h>   // include ShiftPWM.h after setting the pins!
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>


// Function prototypes (telling the compiler these functions exist).
void oneByOne(void);
void inOutTwoLeds(void);
void inOutAll(void);
void alternatingColors(void);
void hueShiftAll(void);
void randomColors(void);
void fakeVuMeter(void);
void rgbLedRainbow(unsigned long cycleTime, int rainbowWidth);

// Here you set the number of brightness levels, the update frequency and the number of shift registers.
// These values affect the load of ShiftPWM.
// Choose them wisely and use the PrintInterruptLoad() function to verify your load.
unsigned char maxBrightness = 255;
unsigned char pwmFrequency = 75;
unsigned int numRegisters = 5;
unsigned int numOutputs = numRegisters * 8;
unsigned int numRGBLeds = numRegisters * 8 / 3;
unsigned int fadingMode = 0; //start with all LED's off.

unsigned long startTime = 0; // start time for the chosen fading mode

bool hasChangedMode = false;
uint8_t mode = 0;
tmElements_t tm;

void setup() {
  while (!Serial) {
    delay(200);
  }
  delay(200);
  Serial.begin(9600);
  Serial.println();
  Serial.println("-------------------");
  pinMode(modeSwitch, INPUT_PULLUP);

  byte error;

  do {
    Wire.beginTransmission(104);
    error = Wire.endTransmission();
    if (error != 0) {
      delay(500);
      Serial.println("Missing i2c device with address 0x68");
    }
  } while (error != 0);

  Serial.println("Found RTC");

  // Sets the number of 8-bit registers that are used.
  ShiftPWM.SetAmountOfRegisters(numRegisters);

  // SetPinGrouping allows flexibility in LED setup.
  // If your LED's are connected like this: RRRRGGGGBBBBRRRRGGGGBBBB, use SetPinGrouping(4).
  ShiftPWM.SetPinGrouping(1); //This is the default, but I added here to demonstrate how to use the funtion

  ShiftPWM.Start(pwmFrequency, maxBrightness);

  if (! RTC.isRunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    if (getDate(__DATE__) && getTime(__TIME__)) {
      RTC.write(tm);
      Serial.println("Set RTC!");
      Serial.print("Is running: ");
      Serial.println(RTC.isRunning());
    }
  } else {
    Serial.println("RTC already running!");
  }

  RTC.read(tm);

  Serial.print("Date: ");
  Serial.print(tm.Day);
  Serial.print("/");
  Serial.print(tm.Month);
  Serial.print("/");
  Serial.print(tm.Year);

  Serial.println();

  Serial.print("Time: ");
  Serial.print(tm.Hour);
  Serial.print(":");
  Serial.print(tm.Minute);
  Serial.print(":");
  Serial.print(tm.Second);
  mode = 0;
  Serial.println();
  Serial.println("-------------------");
}

void loop() {

  int val = digitalRead(modeSwitch);
  if (val == 0) {
    if (hasChangedMode == false) {
      mode = (mode + 1) % 6;
      hasChangedMode = true;
    }
  } else {
    hasChangedMode = false;
  }

  switch (mode) {
    case 0:
      rgbLedRainbow(10000, 5 * numRGBLeds);
      break;
    case 1:
      rgbLedRainbow(10000, 1 * numRGBLeds);
      break;
    case 2:
      rgbLed(0, 255, 0);
      break;
    case 3:
      rgbLed(0, 0, 255);
      break;
    case 4:
      rgbLed(150, 150, 150);
      break;
    case 5:
      rgbLed(0, 0, 0);
      break;
    default:
      break;
  }
}

bool isLedOn(int led) {
  do {
    RTC.read(tm);

  } while (tm.Minute > 59 || tm.Hour > 23);

  int minutes = tm.Minute;
  int minute_first = minutes % 10;
  int minute_second = (minutes / 10) % 10;

  int hours = tm.Hour;
  int hour_first = hours % 10;
  int hour_second = (hours / 10) % 10;

  switch (led) {
    case 0: // 1 Row
      return bitRead(hour_second, 0);
    case 1:
      return bitRead(hour_second, 1);
    case 2: // 2 Row
      return bitRead(hour_first, 0);
    case 3:
      return bitRead(hour_first, 1);
    case 4:
      return bitRead(hour_first, 2);
    case 5:
      return bitRead(hour_first, 3);
    case 6: // 3 Row
      return bitRead(minute_second, 0);
    case 7:
      return bitRead(minute_second, 1);
    case 8:
      return bitRead(minute_second, 2);
    case 9: // 4 Row
      return bitRead(minute_first, 0);
    case 10:
      return bitRead(minute_first, 1);
    case 11:
      return bitRead(minute_first, 2);
    case 12:
      return bitRead(minute_first, 3);
    default:
      return false;
  }
}

void rgbLedRainbow(unsigned long cycleTime, int rainbowWidth) {
  // Displays a rainbow spread over a few LED's (numRGBLeds), which shifts in hue.
  // The rainbow can be wider then the real number of LED's.
  unsigned long time = millis() - startTime;
  unsigned long colorShift = (360 * time / cycleTime) % 360; // this color shift is like the hue slider in Photoshop.

  for (unsigned int led = 0; led < numRGBLeds; led++) { // loop over all LED's
    int hue = ((led) * 360 / (rainbowWidth - 1) + colorShift) % 360; // Set hue from 0 to 360 from first to last led and shift the hue

    if (isLedOn(led)) {
      ShiftPWM.SetHSV(led, hue, 255, 255, 1); // write the HSV values, with saturation and value at maximum
    } else {
      ShiftPWM.SetRGB(led, 0, 0, 0, 1);
    }
  }
}

void rgbLed(int r, int g, int b) {
  for (unsigned int led = 0; led < numRGBLeds; led++) { // loop over all LED's
    if (isLedOn(led)) {
      ShiftPWM.SetRGB(led, r, g, b, 1);
    } else {
      ShiftPWM.SetRGB(led, 0, 0, 0, 1);
    }
  }
}


/* DATE & TIME



*/

const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

bool getTime(const char *str) {
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getDate(const char *str) {
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}
