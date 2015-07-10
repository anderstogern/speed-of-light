/*
Speed-of-light
By Anders S. Tøgern

Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
http://creativecommons.org/licenses/by-sa/3.0/
*/

#include <TinyWireM.h>
#include <SFE_BMP180.h>
#include <dht.h>
#include <LedControlTiny.h>
#include <JeeLib.h> // https://github.com/jcw/jeelib

SFE_BMP180 pressure;

dht DHT;

#define RHT03_PIN 10

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving

LedControlTiny lc = LedControlTiny(PB0, PB1, PB2); // lc is our object
// pin 2 is connected to the MAX7219 pin 1
// pin 3 is connected to the CLK pin 13
// pin 5 is connected to LOAD pin 12

char last = ' ';

void setup() {
  PRR = bit(PRTIM1); // only keep timer 0 going
  
  ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power
  
  // the zero refers to the MAX7219 number, it is zero for 1 chip
  lc.shutdown(false);// turn off power saving, enables display
  lc.setIntensity(15);// sets brightness (0~15 possible values)
  lc.clearDisplay();// clear screen
  
  if (!pressure.begin()) {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.
    printLongFloat(5);
    while(1); // Pause forever.
  }
  
  for (byte i = 0; i < 8; i++) {
    Sleepy::loseSomeTime(500);
    lc.setChar(7 - i, '.', true);
  }
}

void loop() {
  char status;
  double T, P;
  float v = 0;

  while (DHT.read22(RHT03_PIN) != DHTLIB_OK) {
    lc.setChar(0, last, true);
    Sleepy::loseSomeTime(500);
    lc.setChar(0, last, false);
    Sleepy::loseSomeTime(500);
  }
  
  status = pressure.startTemperature();
  // Wait for the measurement to complete:
  Sleepy::loseSomeTime(status);
  
  status = pressure.getTemperature(T);
  if (status != 0) {
    status = pressure.startPressure(3);
    Sleepy::loseSomeTime(status);
    status = pressure.getPressure(P,T);
  }
  
  if (status != 1) {
    lc.setChar(0, ' ', true);
    lc.setChar(1, ' ', true);
    Sleepy::loseSomeTime(1000);
    return;
  }

  static float c = 29979.2458;
  //n = 1 + 7.86 * 10^(-4) * p / (273 + t) - 1.5 * 10^(-11) * RH * (t^2 + 160)
  float n = 1 + (7.86 * 0.0001 * P / (273.15 + T) - 1.5 * 0.00000000001 * DHT.humidity * (T*T + 160));
  v = c/n;
  printLongFloat(v);
  
  unsigned int v2 = (unsigned int) v;
  unsigned int decimals = (v - v2) * 1000;
  last = decimals - ((unsigned int) (decimals / 10) * 10);
  
  Sleepy::loseSomeTime(2000);
}

void printLongFloat(float num) {
  int ones;
  int tens;
  int hundreds;
  int thousands;
  int tenthousands;

  unsigned int v = (unsigned int) num;
  float diff = num - v;
  diff = diff * 1000;
  
  int fones, ftens, fhundreds;
  
  fones=(int)diff%10;
  diff=diff/10;
  ones=v%10;
  v=v/10;
  
  ftens=(int)diff%10;
  diff=diff/10;
  tens=v%10;
  v=v/10;
  
  fhundreds=(int)diff%10;
  diff=diff/10;
  hundreds=v%10;
  v=v/10;
  
  thousands=v%10;
  v=v/10;
  
  tenthousands=v%10;

  lc.setDigit(7,(byte)tenthousands,false);
  lc.setDigit(6,(byte)thousands,false);
  lc.setDigit(5,(byte)hundreds,false);
  lc.setDigit(4,(byte)tens,false);
  lc.setDigit(3,(byte)ones,false);
  lc.setDigit(2,(byte)fhundreds,true);
  lc.setDigit(1,(byte)ftens,false);
  lc.setDigit(0,(byte)fones,false);
}
