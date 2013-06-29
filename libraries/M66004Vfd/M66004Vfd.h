/*
  M66004Vfd Library - Declaration file
  
  Originally created 22 March 2011

  This file is in the public domain.  
*/

#ifndef M66004_VFD_H
#define M66004_VFD_H

#include <inttypes.h>
#include "Print.h"

// API is designed to be as close to the standard LiquidCrystal library as possible.
class M66004Vfd : public Print {

public:

  // For this display, command codes are intermixed with standard ASCII codes as follows.
  //
  // 00h - 07h - Command: display length
  // 08h - 0Fh - Command: brightness
  // 10h - 1Fh - Command: Turn on underscore cursor at location
  // 20h - 7Eh - Character: Standard ASCII
  //       7Fh - Character: Filled block
  // 80h - 8Fh - Command: Turn off underscore cursor at location
  // 90h - 9Fh - Character: User defined
  // A0h - DFh - Character: Japanese
  // E0h - EFh - Command: cursor location
  // F0h - F3h - Command: display off/on/all pixels on
  // F4h - F5h - Command: auto-increment setting
  // F6h - F7f - Command: display refresh frequency
  // F8h - FBh - Command: output port setting
  // FCh - FFh - Command: Begin setting of user defined character data (second byte has index, then data follows).

  static const uint8_t SET_DISPLAY_LENGTH_CHARACTER = 0x00;
  static const uint8_t SET_BRIGHTNESS_CHARACTER = 0x08;
  static const uint8_t UNDERSCORE_ON_CHARACTER = 0x10;
  static const uint8_t UNDERSCORE_OFF_CHARACTER = 0x80;
  static const uint8_t FIRST_USER_DEFINIED_CHARACTER = 0x90; // Library also allows LCD compatible 0-8 to be used.
  static const uint8_t SET_CURSOR_LOCATION_CHARACTER = 0xE0;
  static const uint8_t DISPLAY_ON_CHARACTER = 0xF0;
  static const uint8_t SET_AUTO_INCREMENT_CHARACTER = 0xF4;
  static const uint8_t SET_REFRESH_FREQUENCY_CHARACTER = 0xF6;
  static const uint8_t SET_OUTPUT_PORT_CHARACTER = 0xF8; // Output ports not connected on HP display.
  static const uint8_t SET_USER_DEFINED_DATA_CHARACTER = 0xFC;
    
  M66004Vfd(uint8_t chipSelectPin, uint8_t clockPin, uint8_t dataPin);
  ~M66004Vfd();
  
  // Methods consistent with LiquidCrystal.
  void begin(uint8_t numColumns, int numRows);

  void clear();
  void home();
  void noDisplay();
  void display();

  void createChar(uint8_t characterIndex, uint8_t pixels[] /* 8 bytes, 1 for each row */);
  void setCursor(uint8_t column, uint8_t row); 
  virtual size_t write(uint8_t character);
  void commandCharacter(uint8_t commandCode); // Commands different from LiquidCrystal.

  // These LiquidCrystal methods are not implemented.
//  void noBlink();
//  void blink();
//  void noCursor();
//  void cursor();
//  void scrollDisplayLeft();
//  void scrollDisplayRight();
//  void leftToRight();
//  void rightToLeft();
//  void autoscroll();
//  void noAutoscroll();

  // Methods new to our class.
  void allDisplay(); // Forces all pixels to be set. Cancel with noDisplay() or display().
  
private:

  // We assumes reference oscillator frequency of 500kHz.
  // Seems to be what the LaserJet 4 boards are using.
  static const uint8_t BUSY_WAIT_TIME_IN_MICROSECONDS = 12; 

  // Pin numbers.
  uint8_t _chipSelectPin;
  uint8_t _clockPin;
  uint8_t _dataPin;

  // Display dimensions.
  uint8_t _numDisplayColumns;
  uint8_t _numDisplayRows;
  uint8_t _numDisplayCharacters;
    
  void sendByte(uint8_t dataToWrite, bool lastByte = true);
};

#endif // M66004_VFD_H

