#include <Wire.h>
#include <Arduino.h>
#include "rgb_lcd.h"

#define STRIDE 0
#define DIST 1
#define TIME 2
#define RUN 3
#define RUN2 4
#define RUN3 5

rgb_lcd lcd;

// backlight of LCD screen
const int colorR = 255;
const int colorG = 0;
const int colorB = 255;

// initialize constants
const double gravity = 9.80665;
const int incButtonPin = 4;
const int nextButtonPin = 3;
const int ledPin = 2;
const int inputDelay = 15;
const int firstInDelay = 100;
const int betweenRepeat = 50;
const int flashLength = 100;
const int startBlink = 34;

// final values
double fStride = 0;
double fDist = 0;
double fTime = 0;
bool calculated = false;
double timePer = 0;
double realXVelocity = 0;
double realYVelocity = 0;
double goalVelocity = 0;

// button states
int incButtonState = 0;
int nextButtonState = 0;

// screen states
int digit = 0;
int curInput = 0;
int screen = 0;
char to_print[4] = {'0', '0', '0', '0'};
int incCounter = 0;
int nextCounter = 0;
int cursorBlink = 0;
int ledTimer = 0;
unsigned long mills = 0;
unsigned long startMills = 0;
int numSteps = 0;
bool hasStepped = 0;

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);

  pinMode(incButtonPin, INPUT);
  pinMode(nextButtonPin, INPUT);
  pinMode(ledPin, OUTPUT);

  lcd.setRGB(colorR, colorG, colorB);
  mills = millis();

  lcd.clear();
}

void setPrint() {
  int ind = 3;
  int i = curInput;
  
  while (ind >= 0) {
    to_print[ind] = (i % 10) + 48;
    i = i / 10;
    ind--;
  }

  cursorBlink += 1;
  if (cursorBlink >= 100) {
    cursorBlink = 0;
  }
  if (cursorBlink < startBlink) {
    to_print[digit] = '_';
  }
}

bool shouldInc() {
  int val = digitalRead(incButtonPin);

  if (val == HIGH) {
    incCounter += 1;
    if ((incCounter >= firstInDelay && incCounter % betweenRepeat == 0) || incCounter - 1 == 0) {
      cursorBlink = startBlink + 1;
      return true;
    }
  }
  else {
    incCounter = 0;
  }

  return false;
}

bool shouldNext() {
  int val = digitalRead(nextButtonPin);

  if (val == HIGH) {
    nextCounter += 1;
    if (nextCounter - 1 == 0) {
      cursorBlink = 0;
      return true;
    }
  }
  else {
    nextCounter = 0;
  }

  return false;
}

void do_print(int decimalPos, char* unit, char separator) {
  int cursorPos = 0;

  for(int i = 0; i < 4; i++) {
    if (i == decimalPos) {
      lcd.setCursor(cursorPos,1);
      lcd.print(separator);
      cursorPos++;
    }
    lcd.setCursor(cursorPos, 1);
    lcd.print(to_print[i]);
    cursorPos++;
  }
  lcd.setCursor(cursorPos + 1, 1);
  lcd.print(unit);
}

void stride() {
  lcd.setCursor(0, 0);
  lcd.print("Stride length:");

  if (shouldInc()) {
    int changeDigit = ceil(pow(10, (3 - digit)));
    if (((curInput / (int) ceil(pow(10, 3 - digit))) % 10) == 9) {
      curInput -= (9 * changeDigit);
    }
    else {
      curInput += changeDigit;
    }
  }
  if (shouldNext()) {
    digit += 1;
  }

  setPrint();
  do_print(3, "cm", '.');

  if (digit > 3) {
    digit = 0;
    screen++;
    fStride = ((double) curInput / 1000.0);
    curInput = 0;
    lcd.clear();
  }
}

void dist() {
  lcd.setCursor(0, 0);
  lcd.print("Distance:");

  if (shouldInc()) {
    int changeDigit = ceil(pow(10, (3 - digit)));
    if (((curInput / (int) ceil(pow(10, 3 - digit))) % 10) == 9) {
      curInput -= (9 * changeDigit);
    }
    else {
      curInput += changeDigit;
    }
  }
  if (shouldNext()) {
    digit += 1;
  }

  setPrint();
  do_print(4, "m", '.');

  if (digit > 3) {
    digit = 0;
    screen++;
    fDist = (double) curInput;
    curInput = 0;
    lcd.clear();
  }
}

void time() {
  lcd.setCursor(0, 0);
  lcd.print("Time:");

  if (shouldInc()) {
    int changeDigit = ceil(pow(10, (3 - digit)));
    if (digit == 2 && (((curInput / (int) ceil(pow(10, 3 - digit))) % 10) == 5)) {
      curInput -= (5 * changeDigit);
    }
    else if (digit != 2 && (((curInput / (int) ceil(pow(10, 3 - digit))) % 10) == 9)) {
      curInput -= (9 * changeDigit);
    }
    else {
      curInput += changeDigit;
    }
  }
  if (shouldNext()) {
    digit += 1;
  }

  setPrint();
  do_print(2, "", ':');

  if (digit > 3) {
    digit = 0;
    screen++;
    fTime = ((curInput / 100) * 60) + (curInput % 100);
    curInput = 0;
    lcd.clear();
  }
}

void calculate() {
  calculated = true;
  double numSteps = ((double) fDist / (double) fStride);
  timePer = ((double) fTime / numSteps);
  goalVelocity = ((double) fDist / (double) fTime);
  goalVelocity = (goalVelocity * 3600) / 1000;
  mills = millis();
  startMills = mills;
}

void handleLight() {
  mills = millis();
  if (mills % int(ceil(timePer * 1000)) <= flashLength) {
    digitalWrite(ledPin, HIGH);
    if (!hasStepped) {
      hasStepped = true;
      numSteps++;
    }
  }
  else {
    digitalWrite(ledPin, LOW);
    hasStepped = false;
  }
}

void run() {
  if (!calculated) {
    calculate();
  }

  lcd.setCursor(0, 0);
  lcd.print("Speed:");
  lcd.setCursor(7, 0);
  lcd.print(goalVelocity);
  lcd.setCursor(13, 0);
  lcd.print("kph");
  lcd.setCursor(0, 1);
  lcd.print("Step every");
  lcd.setCursor(11, 1);
  lcd.print(timePer);
  lcd.setCursor(15, 1);
  lcd.print("s");

  handleLight();

  if (shouldInc() || shouldNext()) {
    screen = RUN2;
    lcd.clear();
  }
}

void run2() {
  lcd.setCursor(0, 0);
  lcd.print("Steps:");
  lcd.setCursor(7, 0);
  lcd.print(numSteps);
  lcd.setCursor(0, 1);
  lcd.print("Distance: ");
  lcd.setCursor(10, 1);
  lcd.print(numSteps * fStride);
  lcd.setCursor(15, 1);
  lcd.print("m");

  handleLight();

  if (shouldInc() || shouldNext()) {
    screen = RUN3;
    lcd.clear();
  }
}

void run3() {
  lcd.setCursor(0, 0);
  lcd.print("Time:");
  lcd.setCursor(6, 0);
  lcd.print((millis() - startMills) / 1000.0);
  lcd.setCursor(15, 0);
  lcd.print("s");

  handleLight();

  if (shouldInc() || shouldNext()) {
    screen = RUN;
    lcd.clear();
  }
}

void loop() {
  incButtonState = digitalRead(incButtonPin);
  nextButtonState = digitalRead(nextButtonPin);

  if (screen == STRIDE) {
    stride();
  }
  else if (screen == DIST) {
    dist();
  }
  else if (screen == TIME) {
    time();
  }
  else if (screen == RUN) {
    run();
  }
  else if (screen == RUN2) {
    run2();
  }
  else if (screen == RUN3) {
    run3();
  }
}