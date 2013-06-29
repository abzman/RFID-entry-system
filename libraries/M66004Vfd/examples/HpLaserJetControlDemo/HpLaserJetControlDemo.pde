/*
  M66004Vfd and HpLaserJetControl libraries - Demo
 
 Demonstrates the use a control/display panel from a
 HP LaserJet 4 printer.
 
 This sketch:

 * Sets all pixels and flashes the LEDs for approximately 1 second.
 * Prints "Hello, world!" to the display and flahses a heart beat.
 * Sets LEDs on/off based on button presses.
 
 Not all models of printer have the same pinout. For the model tested, 
 the 10 pin dual row connector was connected as follows:
 
 * Pins 1, 2 and/or 3 connected to Arduino ground.
 * Pins 4 and/or 5 connected to Arduino +5V.
 * Pins 6 through 10 connected to Arduino pins 6 through 10 respectively.
 
 Originally created 21 March 2011
 
 This example code is in the public domain.

 http://arduino.cc/playground/Main/M66004Vfd
*/

#include <M66004Vfd.h> // VFD library.
#include <HpLaserJetControl.h> // Button and LED library.

M66004Vfd vfd(8, 6, 7);
HpLaserJetControl control(9, 6, 7, 10);

// Out custom characters.
byte openHeart[8] = {
  0b00000,
  0b01010,
  0b10101,
  0b10001,
  0b01010,
  0b00100,
  0b00000, 
  0b00000
};

byte filledHeart[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};

// Does a flashing sequence with the LEDs.
void StartupLedSequence()
{
  static const uint8_t LED_ON_TIME_IN_MS = 75;
 
  for (uint8_t i=0; i < 5; i++)
  {
    control.setLedState(1, HIGH);
    delay(LED_ON_TIME_IN_MS);
    control.setLedState(1, LOW);
  
    control.setLedState(2, HIGH);
    delay(LED_ON_TIME_IN_MS);
    control.setLedState(2, LOW);
  
    control.setLedState(0, HIGH);
    delay(LED_ON_TIME_IN_MS);
    control.setLedState(0, LOW);
  }
  
  delay(LED_ON_TIME_IN_MS);
    
  for (uint8_t i=0; i < 2; i++)
  {
    delay(LED_ON_TIME_IN_MS);
    control.setLedState(0, HIGH);
    control.setLedState(1, HIGH);
    control.setLedState(2, HIGH);
    delay(LED_ON_TIME_IN_MS * (i*2+1));
    control.setLedState(0, LOW);
    control.setLedState(1, LOW);
    control.setLedState(2, LOW);
  }
}

// We call this each frame to process button presses and update LED's accordingly.
void UpdateButtonsAndLeds()
{
  // Top row of buttons being pressed toggles.
  if (control.wasButtonPressed(6))
  {
    // Toggle LED1.
    control.setLedState(1, (control.getLedState(1) == LOW) ? HIGH : LOW);
  }
  
  if (control.wasButtonPressed(5))
  {
    // Toggle LED0.
    control.setLedState(0, (control.getLedState(0) == LOW) ? HIGH : LOW);
  }

  if (control.wasButtonPressed(4))
  {
    // Toggle LED2.
    control.setLedState(2, (control.getLedState(2) == LOW) ? HIGH : LOW);
  }

  if (control.wasButtonPressed(3))
  {
    // Toggle all LEDs.
    control.setLedState(0, (control.getLedState(0) == LOW) ? HIGH : LOW);
    control.setLedState(1, (control.getLedState(1) == LOW) ? HIGH : LOW);
    control.setLedState(2, (control.getLedState(2) == LOW) ? HIGH : LOW);
  }

  // Second row of buttons currently down sets LED.
  if (control.isButtonPressed(7))
  {
    // Set LED1.
    control.setLedState(1, HIGH);
  }
  
  if (control.isButtonPressed(1))
  {
    // Set LED0.
    control.setLedState(0, HIGH);
  }
  
  if (control.isButtonPressed(2))
  {
    // Set LED2.
    control.setLedState(2, HIGH);
  }
  
  if (control.isButtonPressed(0))
  {
    // Set all LEDs.
    control.setLedState(0, HIGH);
    control.setLedState(1, HIGH);
    control.setLedState(2, HIGH);
  }
  
  // Second row released clears LED state.
  if (control.wasButtonReleased(7))
  {
    // Clear LED1.
    control.setLedState(1, LOW);
  }
  
  if (control.wasButtonReleased(1))
  {
    // Clear LED0.
    control.setLedState(0, LOW);
  }
  
  if (control.wasButtonReleased(2))
  {
    // Clear LED2.
    control.setLedState(2, LOW);
  }
  
  if (control.wasButtonReleased(0))
  {
    // Clear all LEDs.
    control.setLedState(0, LOW);
    control.setLedState(1, LOW);
    control.setLedState(2, LOW);
  }
}

void setup() {

  // Prepare the VFD library for use.
  vfd.begin(16, 1);
 
  // Create open  as custom character 0. Note that this display supports 16 custom characters.
  vfd.createChar(0, openHeart);
  
  // Create filled heart as custom character 1.
  vfd.createChar(1, filledHeart);
 
  if (control.isDeviceAttached()) // Test that isDeviceAttached() works correctly
  {
    // Sets all pixels on the display.
    vfd.allDisplay();
    
    // LED flashing sequence.
    StartupLedSequence();
    
    // Enables normal character display.
    vfd.display();
  }
  
  // Print some text to the display.
  vfd.print("Hello, world!");
}

void loop() {
  
  UpdateButtonsAndLeds();
  
  // Draw heart beat.
  vfd.setCursor(14, 0);
  vfd.write(((millis() % 1000) < 250) ? 1 : 0);
}

