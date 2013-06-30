/*
  4017 matrix scanner
  
  modified from Paul Newton's netduino example using a 595
  
  June 27, 2013
  
  Evan Allen
 
*/


#include <Arduino.h>

int clock = 27;
int input = 25;
int reset = 26;

#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_SR3W.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal_SR3W iLCD(24,23,22);

char last = ' ';
void setup() {
  Serial.begin(9600);
  
   iLCD.begin ( 16, 2 );
  pinMode (clock, OUTPUT);
  pinMode (input, INPUT);
  pinMode (reset, OUTPUT);
  digitalWrite(clock,0);
  rst();
  delay(10);
  iLCD.clear();
  
                    Serial.println("test");
}

void loop() 
{
  rst();
  byte data_in = shiftIn(input,clock,LSBFIRST);
  char result = ' ';
                    switch (data_in)
                    {

                        case 0x08:
                            result = '1';
                            break;
                        case 0x10:
                            result = '2';
                            break;
                        case 0x20:
                            result = '3';
                            break;
                        case 0x09:
                            result = '4';
                            break;
                        case 0x11:
                            result = '5';
                            break;
                        case 0x21:
                            result = '6';
                            break;
                        case 0x0a:
                            result = '7';
                            break;
                        case 0x12:
                            result = '8';
                            break;
                        case 0x22:
                            result = '9';
                            break;
                        case 0x0c:
                            result = '*';
                            break;
                        case 0x14:
                            result = '0';
                            break;
                        case 0x24:
                            result = '#';
                            break;
                    }
                    
                  if(result != last)
                  //Serial.println(data_in,HEX);
                  {
                    if(result != ' ')
                    {
                      
                  Serial.print(result);
                      iLCD.send(result,DATA);
                      //iLCD.setCursor ( 0, 1 );
                    }
                    last = result;
                  }
                delay(10);
}
void rst()
{
    digitalWrite(reset,0);
  delay(1);
  digitalWrite(reset,1);
}
