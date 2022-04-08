#include <Bounce2.h>

#include <deprecated.h>
#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <require_cpp11.h>

#include <EEPROM.h>

/***************************************************
  DFPlayer - A Mini MP3 Player For Arduino
  <https://www.dfrobot.com/index.php?route=product/product&product_id=1121>

 ***************************************************
  This example shows the basic function of library for DFPlayer.

  Created 2016-12-07
  By [Angelo qiao](Angelo.qiao@dfrobot.com)

  GNU Lesser General Public License.
  See <http://www.gnu.org/licenses/> for details.
  All above must be included in any redistribution
 ****************************************************/

/***********Notice and Trouble shooting***************
  1.Connection and Diagram can be found here
  <https://www.dfrobot.com/wiki/index.php/DFPlayer_Mini_SKU:DFR0299#Connection_Diagram>
  2.This code is tested on Arduino Uno, Leonardo, Mega boards.
 ****************************************************/

#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

#define PIN_VOLUME A0
#define SS_PIN SS
#define RST_PIN 2
#define PIN_PREV 4
#define LED 13
#define PIN_PAUSE 5
#define PIN_NEXT 6
#define CARD_SCAN_INTERVAL_MS 300
#define CARD_COUNT 25

#define EEPROM_ADDR_VOLRESTRICTED 1
#define EEPROM_ADDR_MAXVOL 2

MFRC522 mfrc522(SS_PIN, RST_PIN);

DFRobotDFPlayerMini myDFPlayer;
int volume = 15;
int maxVolume = 30;
int playingFolder = 0;
long pressPauseTime = 0;
bool playing = false;
bool restrictedVolumeMode = false;
int startIndex = 0;
int currentFolderCount = 0;
long lastStart = 0;

void printDetail(uint8_t type, int value);
long getCurrentCard();

Bounce buttonPrev = Bounce();
Bounce buttonPause = Bounce();
Bounce buttonNext = Bounce();

void setup()
{
  pinMode(PIN_VOLUME, INPUT);
  pinMode(LED, OUTPUT);
  buttonPrev.attach(PIN_PREV, INPUT_PULLUP);
  buttonPause.attach(PIN_PAUSE, INPUT_PULLUP);
  buttonNext.attach(PIN_NEXT, INPUT_PULLUP);
  Serial1.begin(9600);
  //while(!Serial);
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  mfrc522.PCD_DumpVersionToSerial();


  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  delay(500);
  while (!myDFPlayer.begin(Serial1)) {  //Use softwareSerial to communicate with mp3.
    delay(500);
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
  }
  Serial.println(F("DFPlayer Mini online."));

  myDFPlayer.volume(volume);  //Set volume value. From 0 to 30

  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  // myDFPlayer.EQ(DFPLAYER_EQ_POP);
  //  myDFPlayer.EQ(DFPLAYER_EQ_ROCK);
  //  myDFPlayer.EQ(DFPLAYER_EQ_JAZZ);
  //myDFPlayer.EQ(DFPLAYER_EQ_CLASSIC);
  //myDFPlayer.EQ(DFPLAYER_EQ_BASS);
  //myDFPlayer.loopFolder(2);


  restrictedVolumeMode = EEPROM.read(EEPROM_ADDR_VOLRESTRICTED) == 1;
  maxVolume = EEPROM.read(EEPROM_ADDR_MAXVOL);

}

int updateVolume() {
  digitalWrite(LED, restrictedVolumeMode);
  int maxV = restrictedVolumeMode ? maxVolume : 30;
  int newVolume = map(analogRead(PIN_VOLUME), 1023, 0, 0, maxV);
  if (newVolume != volume) {
    volume = newVolume;
    myDFPlayer.volume(volume);
  }
}

void loop()
{
  static unsigned long timer = millis();
  buttonPrev.update();
  buttonPause.update();
  buttonNext.update();

  if (buttonPrev.fell()) {
    if (millis() - lastStart > 5000) {
      myDFPlayer.previous();
      delay(200);
      myDFPlayer.next();
      lastStart = millis();
    } else if (myDFPlayer.readCurrentFileNumber() != startIndex) {
      Serial.println(myDFPlayer.readCurrentFileNumber());
      myDFPlayer.previous();
      lastStart = millis();
    }
  }

  if (buttonPause.fell()) {
    pressPauseTime = millis();
    while (!buttonPause.read()) {
      buttonPause.update();
      if (millis() - pressPauseTime > 4000) {
        restrictedVolumeMode = !restrictedVolumeMode;
        maxVolume = volume;
        EEPROM.write(EEPROM_ADDR_VOLRESTRICTED, restrictedVolumeMode);
        EEPROM.write(EEPROM_ADDR_MAXVOL, maxVolume);
        updateVolume();
        return;
      }
    }
    if (playing) {
      myDFPlayer.pause();
      playing = false;
    } else {
      myDFPlayer.start();
      playing = true;
    }

  }

  if (buttonNext.fell() && myDFPlayer.readCurrentFileNumber() < currentFolderCount + startIndex - 1) {
    Serial.println(myDFPlayer.readCurrentFileNumber());
    myDFPlayer.next();
    lastStart = millis();
  }

  int card = getCurrentCard();
  if (card && playingFolder != card) {
    Serial.print("Playing: "); Serial.println(card);
    currentFolderCount = myDFPlayer.readFileCountsInFolder(card);
    Serial.print("File Count: "); Serial.println(currentFolderCount);
    playingFolder = card;
    myDFPlayer.loopFolder(playingFolder);
    lastStart = millis();
    delay(500);
    startIndex = myDFPlayer.readCurrentFileNumber();
    Serial.println(startIndex);
    playing = true;
  }

  updateVolume();

  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
}

void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      if (myDFPlayer.readCurrentFileNumber() < currentFolderCount + startIndex - 1) {
        myDFPlayer.next();
        lastStart = millis();
      }
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

long getCurrentCard() {
  static long lastCard = 0;
  static long lastTime = 0;
  static long cardMappings[CARD_COUNT] = { 986293, 1563555, 1465905, 1224405, 1500135, 1219575, 1238685,
                                           1134105, 1238895, 923335, 964033, 1684305, 994609, 829003,
                                           1004941, 885283, 962353, 11588, 6060, 5252, 12764, 932743, 1858080
                                         };

  if (millis() - CARD_SCAN_INTERVAL_MS > lastTime) {
    bool cardPresent = mfrc522.PICC_IsNewCardPresent() || mfrc522.PICC_IsNewCardPresent();
    if (cardPresent && mfrc522.PICC_ReadCardSerial()) {
      long code = 0;
      int counter = 1;
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        code = ((code + mfrc522.uid.uidByte[i]) * counter);
        counter ++;
      }
      lastCard = 0;
      for (int i = 0; i < CARD_COUNT; i++) {
        if (cardMappings[i] == code) {
          lastCard = i + 1;
          break;
        }
      }
      Serial.println(code);
    } else if (!cardPresent) {
      lastCard = 0;
    }
  }
  return lastCard;
}
