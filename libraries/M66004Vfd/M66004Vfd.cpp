/*
  M66004Vfd Library - Implementation

  Originally created 22 March 2011

  This file is in the public domain.  
*/

#include "M66004Vfd.h"
#include "Arduino.h"

M66004Vfd::M66004Vfd(uint8_t chipSelectPin, uint8_t clockPin, uint8_t dataPin)
{
  _chipSelectPin = chipSelectPin;
  _clockPin = clockPin;
  _dataPin = dataPin;

  // Choose reasonable defaults.
  _numDisplayColumns = 16;
  _numDisplayRows = 1;
  _numDisplayCharacters = 16;
  
  // Configure pins.
  digitalWrite(chipSelectPin, HIGH); // Active low.
  pinMode(chipSelectPin, OUTPUT);
  
  digitalWrite(clockPin, LOW);
  pinMode(clockPin, OUTPUT);

  digitalWrite(dataPin, LOW);
  pinMode(dataPin, OUTPUT);
}
  
M66004Vfd::~M66004Vfd()
{
}
  
void M66004Vfd::begin(uint8_t numColumns, int numRows)
{
  // Limit dimensions to what the library currently supports.
  numColumns = (numColumns < 9) ? 9 : numColumns;
  numColumns = (numColumns > 16) ? 16 : numColumns;
  numRows = 1;  
    
  _numDisplayColumns = numColumns;
  _numDisplayRows = numRows;
  _numDisplayCharacters = numColumns * numRows;

  // Initialize the VFD back to our desired default state.
  sendByte(SET_DISPLAY_LENGTH_CHARACTER | (numColumns - 9));
  sendByte(SET_BRIGHTNESS_CHARACTER | 0b111); // Full brightness.
  sendByte(SET_REFRESH_FREQUENCY_CHARACTER | 0); // Opt for fastest refresh rate.
  sendByte(SET_AUTO_INCREMENT_CHARACTER | 1); // Auto increment on.
 
  clear(); // Clear the display and home cursor.
  display(); // Turn the display on.
}
  
void M66004Vfd::clear()
{
  sendByte(SET_CURSOR_LOCATION_CHARACTER | 0);
  for (uint8_t location = 0; location < _numDisplayColumns; location ++)
  {
    sendByte(' ');
    sendByte(UNDERSCORE_OFF_CHARACTER | location);
  }
  sendByte(SET_CURSOR_LOCATION_CHARACTER | 0);
}
  
void M66004Vfd::home()
{
  sendByte(SET_CURSOR_LOCATION_CHARACTER | 0);
}
  
void M66004Vfd::noDisplay()
{
  sendByte(DISPLAY_ON_CHARACTER | 0b00);
}
  
void M66004Vfd::display()
{
  sendByte(DISPLAY_ON_CHARACTER | 0b01);
}

void M66004Vfd::allDisplay()
{
  sendByte(DISPLAY_ON_CHARACTER | 0b11);
}
  
void M66004Vfd::createChar(uint8_t characterIndex, uint8_t pixels[] /* 8 bytes, 1 for each row */)
{
  static const uint8_t CHARACTER_PIXEL_WIDTH = 5;
  static const uint8_t CHARACTER_PIXEL_PRETEND_HEIGHT = 8;
  static const uint8_t CHARACTER_PIXEL_ACTUAL_HEIGHT = 7;
  
  if (characterIndex < 0x0F)
  {
    sendByte(SET_USER_DEFINED_DATA_CHARACTER, false);
    sendByte(characterIndex, false);
    
    // We need to rotate the pixels.
    uint8_t testBit = 0b10000;
    for (uint8_t i=0; i<CHARACTER_PIXEL_WIDTH; i++)
    {
      uint8_t currentByte = 0;
    
      for (uint8_t j=0; j<CHARACTER_PIXEL_PRETEND_HEIGHT; j++)
      {
        currentByte <<= 1;

        if (pixels[j] & testBit)
        {
          currentByte |= 1;
        }
      }
    
      sendByte(currentByte, i==(CHARACTER_PIXEL_WIDTH-1));
    
      testBit >>= 1;
    }
  }
}
  
void M66004Vfd::setCursor(uint8_t column, uint8_t row)
{
  uint8_t location = row*_numDisplayColumns + column;
  if (location >= _numDisplayCharacters)
  {
    location = 0;
  }
  
  sendByte(SET_CURSOR_LOCATION_CHARACTER | location);
}
  
/*virtual*/ size_t M66004Vfd::write(uint8_t character)
{
  // Remap LCD custom characters to our custom characters.
  if (character < 8)
  {
    character += FIRST_USER_DEFINIED_CHARACTER;
  }
  
  if (character >= ' ') // Ignore ASCII control codes.
  {
    // Map anything hitting a hardware control code to draw a solid block.
    if ((character>=0x80 && character<=0x8F) || (character>=0xE0))
    {
      character = 0x7F;  
    }
    
    // Now we can send the character.
    sendByte(character);
  }
}
  
void M66004Vfd::commandCharacter(uint8_t commandCode)
{
  sendByte(commandCode);
}

// Clocks in a byte to the display. 
// We don't worry about the finer timing as digital write is so slow.
void M66004Vfd::sendByte(uint8_t dataToWrite, bool lastByte)
{
  // Select our device (active low).
  digitalWrite(_chipSelectPin, LOW);
  
  // Clock out the bits for the byte.
  for (uint8_t mask = 0b10000000; mask; mask >>= 1)
  {
    // Set output bit.
    digitalWrite(_dataPin, (dataToWrite & mask) ? HIGH : LOW);    
    
    // Clock data.
    digitalWrite(_clockPin, HIGH);
    digitalWrite(_clockPin, LOW);
  }

  // Must wait between bytes.
  delayMicroseconds(BUSY_WAIT_TIME_IN_MICROSECONDS);

  // Deselect our device.
  if (lastByte)
  {
    digitalWrite(_chipSelectPin, HIGH);
  }
}

