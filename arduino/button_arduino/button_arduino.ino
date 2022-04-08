#include <Bounce2.h>

/*
  Copyright (c) 2014-2015 NicoHood
  See the readme for credit to other people.

  Consumer example
  Press a button to play/pause music player

  You may also use SingleConsumer to use a single report.

  See HID Project documentation for more Consumer keys.
  https://github.com/NicoHood/HID/wiki/Consumer-API
*/

#include "HID-Project.h"

const int pinLed = 17;
const int pinPrev = 5;
const int pinPlay = 7;
const int pinNext = 9;

Bounce buttonPrev = Bounce();
Bounce buttonPlay = Bounce();
Bounce buttonNext = Bounce();


bool locked = false;
bool ignorePrev = false;
bool ignorePlay = false;
bool ignoreNext = false;
bool ignoreLock = false;
unsigned long lockCounter = 0;

void setup() {
  pinMode( LED_BUILTIN_RX, OUTPUT);
  pinMode( LED_BUILTIN_TX, OUTPUT);
  pinMode(pinLed, OUTPUT);
  pinMode(pinPrev, INPUT_PULLUP);
  pinMode(pinPlay, INPUT_PULLUP);
  pinMode(pinNext, INPUT_PULLUP);

  buttonPrev.attach(pinPrev, INPUT_PULLUP);
  buttonPlay.attach(pinPlay, INPUT_PULLUP);
  buttonNext.attach(pinNext, INPUT_PULLUP);

  // Sends a clean report to the host. This is important on any Arduino type.
  Consumer.begin();
  //Serial.begin(9600);
}

void sendMedia(unsigned int key) {
  if (locked) {
    return;
  }
  Consumer.write(key);
}

void loop() {
  //digitalWrite( LED_BUILTIN_RX, HIGH);
  //digitalWrite( LED_BUILTIN_TX, HIGH);
  digitalWrite(pinLed, !locked);
  buttonPrev.update();
  buttonPlay.update();
  buttonNext.update();

  if (buttonPrev.read() == LOW && buttonNext.read() == LOW && !ignoreLock) {
    lockCounter++;
  }

  if (lockCounter > 40000) {
    //Serial.println(lockCounter);
    //Serial.println(locked);
    locked = !locked;
    ignoreLock = true;
    ignoreNext = true;
    ignorePrev = true;
    lockCounter = 0;
  }
  if (buttonPrev.rose()) {
    lockCounter = 0;
    ignoreLock = false;
    if (ignorePrev) {
      ignorePrev = false;
      return;
    }
    if (buttonPlay.read() == LOW) {
      sendMedia(MEDIA_VOLUME_DOWN);
      ignorePlay = true;
    } else {
      sendMedia(MEDIA_PREVIOUS);
    }
    //Serial.println("buttonPrev");
  }
  if (buttonPlay.rose()) {
    //Serial.println("buttonPlay");
    if (!ignorePlay) {
      sendMedia(MEDIA_STOP);
    }
    ignorePlay = false;
  }
  if (buttonNext.rose()) {
    ignoreLock = false;
    lockCounter = 0;
    if (ignoreNext) {
      ignoreNext = false;
      return;
    }
    if (buttonPlay.read() == LOW) {
      sendMedia(MEDIA_VOLUME_UP);
      ignorePlay = true;
    } else {
      sendMedia(MEDIA_NEXT);
    }
    //Serial.println("buttonNext");
  }
}
