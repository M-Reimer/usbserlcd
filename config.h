// Note: It is not recommended to change here!
// If at all possible just wire your LCD to your Arduino in exactly this
// way so your hardware keeps compatible to the "default firmware"

// Data bits
const int PIN_D0 = 2;
const int PIN_D1 = 3;
const int PIN_D2 = 4;
const int PIN_D3 = 5;
const int PIN_D4 = 6;
const int PIN_D5 = 7;
const int PIN_D6 = A0;
const int PIN_D7 = A1;

// Status bits
const int PIN_CE = A2;
const int PIN_WR = A3;
#if defined(__AVR_ATmega32U4__)
const int PIN_CD = 14; // Use these pins for the Arduino Pro Micro (Mega32u4) board
const int PIN_RD = 15;
#else
const int PIN_CD = A4; // Use these for the Arduino Nano with FT232RL serial converter
const int PIN_RD = A5;
#endif

// Output pin (PWM) to control backlight brightness
// You need a PN2222A transistor (or similar) with an 1K base resistor to drive the LED backlight!
const int PIN_BRIGHTNESS = 9;

// This pin is meant to be wired (with 1K series resistor) to the PWREN# (CBUS3, Pin 14) pin of an FT232RL IC.
// This allows the LCD to be turned off if you shutdown the PC even if the 5V usb voltage is not turned off.
// This feature only works if you use an Arduino board which is equipped with the FT232RL serial converter IC.
// If you don't need this feature, please wire this pin directly to GND (always on).
const int PIN_SLEEP = 12;

