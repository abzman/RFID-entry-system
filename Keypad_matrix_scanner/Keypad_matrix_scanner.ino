/*
  4017 matrix scanner
  
  modified from Paul Newton's netduino example using a 595
  
  June 27, 2013
  
  Evan Allen
 
*/


#include <Arduino.h>

int clock = 49;
int input = 50;

void setup() {
  Serial.begin(9600);
  
  pinMode (clock, OUTPUT);
  pinMode (input, INPUT);
  digitalWrite(clock,0);
  delay(10);
}

void loop() 
{
  
  byte data_in = shiftIn(input,clock,LSBFIRST);
  
  clk();
  clk();
  char result = ' '; // need to define something ...
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
                    //Serial.println(data_in,HEX);
                    Serial.println(result);
                delay(10);
}
void clk()
{
    digitalWrite(clock,1);
  delay(1);
  digitalWrite(clock,0);
}
