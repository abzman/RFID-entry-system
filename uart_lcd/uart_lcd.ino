/*
  UART controlled LCD and keypad
 
 Uses a standard HD44780 LCD and a keypad 
 matrix to talk to a master device over serial.
 
 This sketch prints "lcd controller" to the LCD
 and waits for further commands.
 
 The circuit:
 * LCD RS pin to digital pin 2
 * LCD Enable pin to analog pin 1
 * LCD D4 pin to digital pin 9
 * LCD D5 pin to digital pin 10
 * LCD D6 pin to digital pin 12
 * LCD D7 pin to digital pin 13
 * LCD R/W pin to ground
 * LCD VO pin to digital pin 6 (pwm)
 * LCD BACKLIGHT pin to digital pin 11 (pwm) via transistor
 
 First functional code published:
 01/06/2014 by Evan Allen
 
 This code depends on a specially tuned 
 SerialCommand library with values picked
 for the 328 in the header file. 
 
 This code makes use of the standard LiquidCrystal library 
 for output and the keypad library version 3.1 by 
 Mark Stanley, and Alexander Brevig.  
 */


// include the library code:
#include <SerialCommand328.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <Keypad.h>

// lcd stuff
LiquidCrystal lcd(2, A1, 9, 10, 12, 13);
int backlight_pin = 11;
int back_value = 255;
const int back_default = 255;
int contrast_pin = 6;
int cont_value = 90;
const int cont_default = 90;

SerialCommand328 SCmd;   // The SerialCommand object

//keypad stuff
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}};
byte rowPins[ROWS] = {A2,A0,4,5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {7,3,8}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );



void setup() {
  Serial.begin(115200);
  LCD_setup();
  lcd.print("lcd controller");
  SCmd.addCommand("DISPLN",SCmd_displayln);  // uses display
  //format: DISPLN;<0=line 1, 1=line 2 (or no argument to clear the display)>;<string (or no argument to clear the line)>
  SCmd.addCommand("DISP",SCmd_display);  // uses display from current cursor position
  //format: DISP;<string>
  SCmd.addCommand("BACKLIGHT",SCmd_backlight);  // sets backlight
  //format: BACKLIGHT;<analog value, 0-255>
  SCmd.addCommand("CONTRAST",SCmd_contrast);  // sets contrast
  //format: CONTRAST;<analog value, 0-255>
  SCmd.addDefaultHandler(unrecognized);  // Handler for command that isn't matched  (does nothing) 
}

void LCD_setup()
{
  pinMode(contrast_pin,OUTPUT);
  pinMode(backlight_pin,OUTPUT);
  analogWrite(backlight_pin,back_value);
  analogWrite(contrast_pin,cont_value);
  lcd.begin(16, 2);  // set up the LCD's number of columns and rows
}

void loop() {  
  SCmd.readSerial();     //process serial commands

  char key = keypad.getKey();

  if (key){
    Serial.print(key);  //if there was a key, bark it out over serial, let their buffer handle it from there
  }
}

// uses display
//format: DISPLN;<0=line 1, 1=line 2 (or no argument to clear the display)>;<string (or no argument to clear the line)>
void SCmd_displayln()    
{
  char *arg; 
  arg = SCmd.next();
  if (arg != NULL) 
  {
    if (atoi(arg) == 0)
    {
      arg = SCmd.next();
      if (arg != NULL) 
      {
        LCD_displn(arg,0);
      } 
      else
      {
        LCD_displn("                ",0);
      }
    }
    else if (atoi(arg) == 1)
    {
      arg = SCmd.next();
      if (arg != NULL) 
      {
        LCD_displn(arg,1);
      } 
      else
      {
        LCD_displn("                ",1);
      }
    }
  }  
  else 
  {
    LCD_clear();
  }
}

// uses display from current cursor position
//format: DISP;<string>
void SCmd_display()    
{
  char *arg; 
  arg = SCmd.next();
  if (arg != NULL) 
  {
    LCD_disp(arg);
  }
}


// sets backlight
//format: BACKLIGHT;<analog value, 0-255>
void SCmd_backlight()
{
  char *arg; 
  arg = SCmd.next(); 
  if (arg != NULL) 
  {
    back_value = atoi(arg);
  }
  else
  {
    //set backlight to default value
    back_value = back_default;
  }
  analogWrite(backlight_pin,back_value);
}


// sets contrast
//format: CONTRAST;<analog value, 0-255>
void SCmd_contrast()    
{
  char *arg; 
  arg = SCmd.next(); 
  if (arg != NULL) 
  {
    cont_value = atoi(arg);
  }
  else
  {
    //set contrast to default value
    cont_value = cont_default;
  }
  analogWrite(contrast_pin,cont_value);
}

// This gets set as the default handler, and gets called when no other command matches. 
void unrecognized()
{
}

//displays on a given line
void LCD_displn(String prints,int line)
{
  lcd.setCursor(0,line); // position cursor
  LCD_disp(prints);  // text
}

//displays from the current cursor position
void LCD_disp(String prints)
{
  char printable[20];
  prints.toCharArray(printable,sizeof(printable));
  lcd.write(printable);  // text
}

//clears the lcd (wrapper for portability)
inline void LCD_clear()
{
  lcd.clear(); //clear screen
}
