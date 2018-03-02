/*
  USBserLCD Parallel T6963C LCD to USB converter
  Copyright (C) 2016 Manuel Reimer <manuel.reimer@gmx.de>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma GCC optimize ("O3")

#include "config.h"
#include "digitalWriteFast.h"
#include "splashscreen.h"

// T6963 commands
const unsigned char kSetAddressPointer = 0x24;
const unsigned char kSetGraphicHomeAddress = 0x42;
const unsigned char kSetGraphicArea        = 0x43;
const unsigned char kSetMode          = 0x80;
const unsigned char kSetDisplayMode   = 0x90;
const unsigned char kDataWriteInc = 0xC0;
const unsigned char kAutoWrite = 0xB0;
const unsigned char kAutoReset = 0xB2;

// T6963 Parameters
const unsigned char kModeOr            = 0x00;
const unsigned char kModeInternalCG    = 0x00;
const unsigned char kDisplayModeGraphic = 0x08;
const unsigned short kGraphicBase = 0x0000;

// Possible serial package types
enum glcd_pkgtype: char {
  PKGTYPE_DATA,
  PKGTYPE_BRIGHTNESS
};

// Sets direction of the 8-bit parallel interface
void ParportSetDirection(int aPinMode) {
  pinMode(PIN_D0, aPinMode);
  pinMode(PIN_D1, aPinMode);
  pinMode(PIN_D2, aPinMode);
  pinMode(PIN_D3, aPinMode);
  pinMode(PIN_D4, aPinMode);
  pinMode(PIN_D5, aPinMode);
  pinMode(PIN_D6, aPinMode);
  pinMode(PIN_D7, aPinMode);
}

// Set one byte to the parallel interface
void ParportWriteData(unsigned char aData) {
  digitalWriteFast(PIN_D0, aData & (1<<0));
  digitalWriteFast(PIN_D1, aData & (1<<1));
  digitalWriteFast(PIN_D2, aData & (1<<2));
  digitalWriteFast(PIN_D4, aData & (1<<4));
  digitalWriteFast(PIN_D5, aData & (1<<5));
  digitalWriteFast(PIN_D6, aData & (1<<6));
  digitalWriteFast(PIN_D7, aData & (1<<7));
  digitalWriteFast(PIN_D3, aData & (1<<3));
  // Give the LCD some time to fetch the bits
  __asm__ __volatile__ ("nop\n\t");
}

// Blocks until LCD reports DSP to be ready
// FIXME: Someone has to check this one.
//        Why is "INPUT_PULLUP" needed?
//        Why is reading status *that* slow?
//        Disabled and replaced by static delay
void WaitDSPReady(bool aAutoWrite = false) {
  for (char s = 0; s < 20; s++)
    __asm__ __volatile__ ("nop\n\t");
// Currently disabled (read FIXME above).
/*
  ParportSetDirection(INPUT_PULLUP);

  digitalWriteFast(PIN_CD, HIGH);
  digitalWriteFast(PIN_RD, LOW);
  digitalWriteFast(PIN_WR, HIGH);
  digitalWriteFast(PIN_CE, LOW);

  for (int i = 0; i < 60; i++) {
    // In "Non-Auto-Mode" check STA0 and STA1
    if (!aAutoWrite && digitalReadFast(PIN_D0) && digitalReadFast(PIN_D1))
      break;
    // In "Auto-Mode" check STA3
    if (aAutoWrite && digitalReadFast(PIN_D3))
      break;
  }

  digitalWriteFast(PIN_CE, HIGH);
  digitalWriteFast(PIN_RD, HIGH);
  digitalWriteFast(PIN_CD, LOW);

  ParportSetDirection(OUTPUT);  
*/
}

// Sends Data to LCD controller
void SendData(unsigned char aData, bool aAutoWrite = false) {
  WaitDSPReady(aAutoWrite);
  digitalWriteFast(PIN_CD, LOW);  // CD down (data)
  digitalWriteFast(PIN_WR, LOW);  // CD & WR down
  digitalWriteFast(PIN_CE, LOW);
  ParportWriteData(aData);
  digitalWriteFast(PIN_CE, HIGH); // CE & WR up again
  digitalWriteFast(PIN_WR, HIGH);
  digitalWriteFast(PIN_CD, HIGH); // CD up again
}

// Sends command to LCD controller
void SendCommand(unsigned char aCmd) {
  WaitDSPReady();
  digitalWriteFast(PIN_CD, HIGH); // CD up (command)
  digitalWriteFast(PIN_WR, LOW);  // CD & WR down
  digitalWriteFast(PIN_CE, LOW);
  ParportWriteData(aCmd);
  digitalWriteFast(PIN_CE, HIGH); // CE & WR up again
  digitalWriteFast(PIN_WR, HIGH);
  digitalWriteFast(PIN_CD, LOW); // CD down again
}

void CommandWord(unsigned char cmd, unsigned short data) {
  SendData(data % 256);
  SendData(data >> 8);
  SendCommand(cmd);
}

// Initializes pins
void InitPins() {
  // Status bits are all outputs
  pinModeFast(PIN_RD, OUTPUT);
  pinModeFast(PIN_WR, OUTPUT);
  pinModeFast(PIN_CE, OUTPUT);
  pinModeFast(PIN_CD, OUTPUT);
  // Brightness-Pin is output
  pinModeFast(PIN_BRIGHTNESS, OUTPUT);
  // Sleep-Pin is input
  pinModeFast(PIN_SLEEP, INPUT);
  // Initial status: Disable chip, Disable reading, Disable writing, Command mode
  digitalWriteFast(PIN_RD, HIGH);
  digitalWriteFast(PIN_WR, HIGH);
  digitalWriteFast(PIN_CE, HIGH);
  digitalWriteFast(PIN_CD, HIGH);
  // Initialize parallel port in output mode
  ParportSetDirection(OUTPUT);
}

// Initializes display controller
void InitLCD() {
  CommandWord(kSetGraphicHomeAddress, kGraphicBase);
  CommandWord(kSetGraphicArea, 240 / 8);
  SendCommand(kSetMode | kModeOr | kModeInternalCG);
  SendCommand(kSetDisplayMode | kDisplayModeGraphic);
}

// Displays splashscreen image
void DisplaySplashscreen() {
  CommandWord(kSetAddressPointer, kGraphicBase);
  SendCommand(kAutoWrite);
  for (int index = 0; index < (240*128)/8; index++) {
    unsigned char byte = pgm_read_byte_near(SplashImage + index);
    SendData(byte, true);
  }
  SendCommand(kAutoReset);
}

// Clears the screen
void ClearScreen() {
  CommandWord(kSetAddressPointer, kGraphicBase);
  SendCommand(kAutoWrite);
  for (int index = 0; index < (240*128)/8; index++) {
    SendData(0, true);
  }
  SendCommand(kAutoReset);
}

// This function checks for changes on the sleep pin
// If LCD turns on, start brightness and splashscreen are set
// If LCD turns off, backlight is turned off and screen is cleared
void CheckSleep() {
  static int lastSleepValue = HIGH;
  const int SleepValue = digitalRead(PIN_SLEEP);
  if (SleepValue != lastSleepValue) {
    if (SleepValue == LOW) {
      analogWrite(PIN_BRIGHTNESS, 255);
      DisplaySplashscreen();
    }
    else {
      analogWrite(PIN_BRIGHTNESS, 0);
      ClearScreen();
    }
    lastSleepValue = SleepValue;
  }
}

// Reads on the serial port until the header bytes are found.
// Returns the byte following the header bytes which specifies the type of this data block
glcd_pkgtype SyncToSerialHeader() {
  char in_byte = 0;
  while(true) {
    CheckSleep();
    if (in_byte != 'G') {
      Serial.readBytes(&in_byte, 1);
      continue;
    }
    if (!Serial.readBytes(&in_byte, 1))
      continue;
    if (in_byte != 'L')
      continue;
    if (!Serial.readBytes(&in_byte, 1))
      continue;
    if (in_byte != 'C')
      continue;
    if (!Serial.readBytes(&in_byte, 1))
      continue;
    if (in_byte != 'D')
      continue;
    if (!Serial.readBytes(&in_byte, 1))
      continue;
    return (glcd_pkgtype)in_byte;
  }
}

void setup() {
  Serial.begin(150000);
  InitPins();
  InitLCD();
  CheckSleep();

/*
  // (Speed-)Test sequence. Prints bit-pattern to LCD
  unsigned char byte = 0;
  const int max = (128*240)/8;
  do {
    CommandWord(kSetAddressPointer, kGraphicBase);
    SendCommand(kAutoWrite);
    for (int index = 0; index < max; index++)
    SendData(byte, true);
    SendCommand(kAutoReset);
  } while (byte++ != 255);
*/
}

void loop() {
  glcd_pkgtype pkgtype = SyncToSerialHeader();
  switch(pkgtype) {
    // Package is data.
    // Read address and length first.
    // Then stream following bytes directly to the LCD controller
    case PKGTYPE_DATA: {
      unsigned short address = 0;
      unsigned short length = 0;
      if (!(Serial.readBytes((char*)&address, 2) == 2))
        return;
      if (!(Serial.readBytes((char*)&length, 2) == 2))
        return;
      CommandWord(kSetAddressPointer, kGraphicBase + address);
      SendCommand(kAutoWrite);
      for (unsigned short index = 0; index < length; index++) {
        char in_byte = 0;
        if (!Serial.readBytes(&in_byte, 1))
          break;
        SendData(in_byte, true);
      }
      SendCommand(kAutoReset);
      break;
    }
    // Package is brightness
    // Read expected brightness and set analog port
    case PKGTYPE_BRIGHTNESS: {
      unsigned char brightness = 0;
      if (!Serial.readBytes(&brightness, 1))
        break;
      analogWrite(PIN_BRIGHTNESS, brightness);
      break;
    }
  }
}

