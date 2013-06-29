/*
  HpLaserJetControl Library - Implementation

  Originally created 21 March 2011

  This file is in the public domain.  
*/

#include "HpLaserJetControl.h"
#include "Arduino.h"

HpLaserJetControl::HpLaserJetControl(uint8_t latchPin, uint8_t clockPin, uint8_t dataWritePin, uint8_t dataReadPin)
{
  _latchPin = latchPin;
  _clockPin = clockPin;
  _dataWritePin = dataWritePin;
  _dataReadPin = dataReadPin;  
  
  // Initialize pins.  
  digitalWrite(_clockPin, LOW);
  pinMode(_clockPin, OUTPUT);

  digitalWrite(_dataWritePin, LOW);
  pinMode(_dataWritePin, OUTPUT);

  digitalWrite(_latchPin, LOW);
  pinMode(_latchPin, OUTPUT);

  pinMode(_dataReadPin, INPUT);
  digitalWrite(_dataReadPin, LOW);

  // Init LED's off.
  _currentLedValues = LEDS_OFF_BYTE; // Top 3 bits are for LEDs, inverted.
  flushLedBits();
   
  // Init button state.
  _currentButtonState = readButtonState();
  _pressedState = 0;
  _releasedState = 0; 
  _lastReadMark = millis() - LAST_READ_MARK_OFFSET_FUDGE;
}

bool HpLaserJetControl::isButtonPressed(uint8_t buttonIndex)
{
  updateButtons();
  
  uint8_t bitMask = (1 << buttonIndex);
  return (_currentButtonState & bitMask) != 0;
}

bool HpLaserJetControl::wasButtonPressed(uint8_t buttonIndex)
{
  updateButtons();
  
  uint8_t bitMask = (1 << buttonIndex);
  uint8_t pressedStateTmp = _pressedState;
  _pressedState &= ~bitMask;
  return (pressedStateTmp & bitMask) != 0;
}

bool HpLaserJetControl::wasButtonReleased(uint8_t buttonIndex)
{
  updateButtons();
  
  uint8_t bitMask = (1 << buttonIndex);
  uint8_t releasedStateTmp = _releasedState;
  _releasedState &= ~bitMask;
  return (releasedStateTmp & bitMask) != 0;
}

uint8_t HpLaserJetControl::getLedState(int8_t ledIndex)
{
  uint8_t bitMask = 1 << (ledIndex + 5); // Led's are set by top 3 bits of shift register.
  return (bitMask & _currentLedValues) ? LOW : HIGH; // Led on/off is inverted.
}

void HpLaserJetControl::setLedState(uint8_t ledIndex, uint8_t level)
{
  uint8_t currentLedValuesTmp = _currentLedValues;
  uint8_t bitMask = (1 << ledIndex + 5); // Led's are set by top 3 bits of shift register.
  if (level == LOW)
  {
    // Led on/off is inverted.
    currentLedValuesTmp |= bitMask;
  }
  else
  {
    // Led on/off is inverted.
    currentLedValuesTmp &= ~bitMask;
  }
  
  if (currentLedValuesTmp != _currentLedValues)
  {
    _currentLedValues = currentLedValuesTmp;
    flushLedBits();
  }    
}

// Pings the device to see if it is attached.
// We simply clock two bytes through the 2 connected 8 bit
// shift registers located on the board.
bool HpLaserJetControl::isDeviceAttached()
{
  bool isAttached = true;

  // Set LED bits to be correct since something else may have clocked in
  // different values.
  clockByte(_currentLedValues);
  
  // Latch so we are able to clock our bytes through the shift register.
  digitalWrite(_latchPin, HIGH);    
  
  clockByte(PING_TEST_BYTE_0);
  clockByte(PING_TEST_BYTE_1);
  
  if (clockByte(_currentLedValues)!=PING_TEST_BYTE_0)
  {
    isAttached = false;
  }
  
  if (clockByte(_currentLedValues)!=PING_TEST_BYTE_1)
  {
    isAttached = false;
  }

  // Restore latch back to low.
  digitalWrite(_latchPin, LOW);    
     
  return isAttached;
}

// Clocks a byte out to the serial-in, parallel out chip, while at
// the same time, clocks in a byte from the parallel in, serial out chip.
uint8_t HpLaserJetControl::clockByte(uint8_t dataToWrite)
{
  // Data is cocked MSB to LSB.
  uint8_t dataRead = 0;
  
  for (uint8_t mask = 0b10000000; mask; mask >>= 1)
  {
    // Read in current bit.
    dataRead <<= 1;
    if (digitalRead(_dataReadPin) != LOW)
    {
      dataRead |= 0b00000001;
    }
   
    // Set output bit.
    digitalWrite(_dataWritePin, (dataToWrite & mask) ? HIGH : LOW);    
    
    // Clock data.
    digitalWrite(_clockPin, HIGH);
    digitalWrite(_clockPin, LOW);
  }
  
  return dataRead;
}

void HpLaserJetControl::flushLedBits()
{
  clockByte(_currentLedValues);
  
  // Low to high will latch (present) data to the LEDs. 
  // This will also latch in the button inputs.
  digitalWrite(_latchPin, HIGH);    
  
  // Restore clock line low so further clocking will have no effect on LED states.
  digitalWrite(_latchPin, LOW);
}

uint8_t HpLaserJetControl::readButtonState()
{
  // Set LED bits to be correct since something else may have clocked in
  // different values.
  clockByte(_currentLedValues);

  // Latch button state. This will also latch in the LED values. 
  digitalWrite(_latchPin, HIGH);
    
  // Read in latched state, clocking in the current LED values again.
  uint8_t buttonValues = clockByte(_currentLedValues);

  // Restore latch back to low.
  digitalWrite(_latchPin, LOW); 
   
  return buttonValues;
}

void HpLaserJetControl::updateButtons()
{
  if ((millis() - _lastReadMark) >= BUTTON_DEBOUNCE_DELAY_IN_MS)
  {
    // It has been long enough since the last time we read the button state.
    
    uint8_t newButtonState = readButtonState();
    if (newButtonState != _currentButtonState)
    {
      // Button state has changed.
      // Figure out pressed and released states.
      uint8_t changedBits = newButtonState ^ _currentButtonState;
      _pressedState |= changedBits & newButtonState;
      _releasedState |= changedBits & _currentButtonState;
      
      // Save off new state.
      _currentButtonState = newButtonState;
      
      // Set _lastReadMark so we get the full debounce wait.
      _lastReadMark = millis();
    }
    else
    {
      // Set the _lastReadMark so we get minimum delay before next read.
      _lastReadMark = millis() -  - LAST_READ_MARK_OFFSET_FUDGE;
    }
  }
}


