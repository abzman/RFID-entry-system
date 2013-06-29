/*
  HpLaserJetControl Library - Declaration file
  
  Originally created 21 March 2011

  This file is in the public domain.  
*/

#ifndef HpLaserJetControl_h
#define HpLaserJetControl_h

#include <inttypes.h>

// This class provides an interface to the buttons on a HP LaserJet 4 control/display board.
class HpLaserJetControl {
public:

  HpLaserJetControl(uint8_t latchPin, uint8_t clockPin, uint8_t dataWritePin, uint8_t dataReadPin);

  // All button reads are debounced.
  bool isButtonPressed(uint8_t buttonIndex); 
  bool wasButtonPressed(uint8_t buttonIndex); // Latched, clears state for button when read.
  bool wasButtonReleased(uint8_t buttonIndex); // Latched, clears state for button when read.
  
  uint8_t getLedState(int8_t ledIndex);
  void setLedState(uint8_t ledIndex, uint8_t level);

  bool isDeviceAttached();

private:

  // These are clocked into the shift registers and then read out to identify
  // whether a board exists.
  static const uint8_t PING_TEST_BYTE_0 = 103; // Just a random number.
  static const uint8_t PING_TEST_BYTE_1 = 242; // Just another random number.

  static const uint8_t LEDS_OFF_BYTE = 0xFF; // LEDs are active low.

  static const uint8_t MIN_BUTTON_READ_DELAY_IN_MS = 1; // Limit rate of polling to minimize CPU time, etc.
  static const uint8_t BUTTON_DEBOUNCE_DELAY_IN_MS = 20;
  static const uint8_t LAST_READ_MARK_OFFSET_FUDGE = BUTTON_DEBOUNCE_DELAY_IN_MS - MIN_BUTTON_READ_DELAY_IN_MS;

  // Pin numbers.
  uint8_t _clockPin;
  uint8_t _dataWritePin;
  uint8_t _latchPin;
  uint8_t _dataReadPin;

  // LED state.
  uint8_t _currentLedValues;

  // Button reading state.
  unsigned long _lastReadMark;
  uint8_t _currentButtonState;
  uint8_t _pressedState;
  uint8_t _releasedState;
  
  uint8_t clockByte(uint8_t dataToWrite);
  void flushLedBits();
  uint8_t readButtonState();
  void updateButtons();
};
  
#endif // HpLaserJetControl_h

