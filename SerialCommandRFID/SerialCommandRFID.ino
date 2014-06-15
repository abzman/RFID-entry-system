// RFID project
// i3 Detroit
// Evan Allen
// January 2014

#define PRIV_ACCESS 0   //allowed to enter space
#define PRIV_ADDUSER 1  //allowed to add or remove users
#define PRIV_DOORBELL 2 //allowed to set doorbell mode
#define PRIV_DOORHOLD 3 //allowed to set door hold mode

#include <MD5.h>
#include <Arduino.h>
#include <String.h>


//library to allow it to take commands from serial
#include <SerialCommand2560.h>
SerialCommand2560 SCmd;   // The demo SerialCommand object


// Interface to DS1822 chip, record data to EEPROM
#include <OneWire.h>
OneWire ds(30);  // define the pin the interface is on */
float current_temperature;

//I2C stuff
#include <Wire.h>         // Needed for I2C Connection to the DS1307 date/time chip
#include <DS1307.h>       // DS1307 RTC Clock/Date/Time chip library
#include <E24C1024.h>     // Needed for the onboard EEPROM
uint8_t second, minute, hour, dayOfWeek, dayOfMonth, month, year;     // Global RTC clock variables. Can be set using DS1307.getDate function.
DS1307 ds1307;        // RTC Instance


//RFID reader stuff
#include <WIEGAND26.h>    // Wiegand 26 reader format library
byte reader1Pins[]={
  19,18};          // Reader 1 connected to pins 19,18
byte reader2Pins[]={
  2,3};          // Reader2 connected to pins 2,3
long reader1 = 0;
int  reader1Count = 0;
long reader2 = 0;
int  reader2Count = 0;
const byte reader2buzz = 6; //pin for the buzzer on Reader2
const byte reader2led = 4; //pin for led1 on Reader2
const byte reader2led2 = 5; //pin for led2 on Reader2
const byte reader1buzz = 7; //pin for the buzzer on Reader1
const byte reader1led = 8; //pin for led1 on Reader1
const byte reader1led2 = 17; //pin for led2 on Reader1
WIEGAND26 wiegand26;


//Get the interrupt number of a given digital pin on an Arduino MEGA.
const byte pinToInterrupt[22] = {
  -1,-1,0,1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,5,4,3,2};


//pin-mappings
const int relaysToPins[8] = {
  31,32,33,34,35,36,37,38};
const int analogToPins[16] = {
  54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69};


//exit button stuff
const int exit_button = 48; //digital pin the exit button is attached to
const int exit_relay = 0; //relay the door solenoid is on
const unsigned long exit_time = 3000; //time to keep the door open in thousandths of a second
boolean exiting = 0; //whether or not the door should be open (and timer running)
unsigned long exit_start_time = 0; //used for count down timer


//RFID timer stuff
const unsigned long RFID_time = 30000; //timeout for the RFID scan in thousandths of a second
unsigned long RFID_start_time = 0; //used for the count down timer


//dot matrix printer stuff
const int dataPins[] = {
  41,40,43,42,45,44,47,46};
const int strobePin = 39;//active low
const int busyPin = 49;//active low


//LCD stuff
int back_val = 255;
int cont_val = 90;
String transmit = "";
String printname = "";

//EEPROM stuff
//if I have time I should figure out why it errors out reading over 512 entries (should get 2048)
int entry_size = 64;
byte hash[32];
//store card id (currently a long, might not need to be)
char name[31];
byte priv;
byte command[64]; //add time zone


//doorbell mode stuff
boolean doorbell = 0; //in doorbell mode?
boolean can_doorbell = 0; //can a given user enter doorbell mode?
unsigned long doorbell_start = 0; //used for count down timer
unsigned long doorbell_dur = (1000*60*60*6); //timeout for doorbell in hours (default 6)


//toggle stuff
//unsigned long max_toggle = 15000; //maximum amount of time a toggle can seize up the processor with a delay loop
unsigned long toggle_dur[8] = {
  0,0,0,0,0,0,0,0}; //time in thousandths of a second
unsigned long toggle_start[8] = {
  0,0,0,0,0,0,0,0}; //start time
boolean toggled[8] = {
  0,0,0,0,0,0,0,0}; //active or not


//door hold mode stuff
boolean doorhold = 0; //in door hold mode?
boolean can_doorhold = 0; //can a given user enter door hold mode?
unsigned long doorhold_start = 0; //used for count down timer
unsigned long doorhold_dur = (1000*60*10); //timeout for door hold in minutes


//battery stuff
float voltage_threshold = 13.6; //threshold to throw an error at
unsigned long battery_start = 0; //used for count down timer
unsigned long battery_dur = (1000*60*15); //time between battery checks (fifteen minutes default)

long tag = 0; //stores tag
boolean tag_read = 0; //in pin reading mode
byte pin[8] = {
  15,15,15,15,15,15,15,15}; //untypeable character, unlike 0
byte digit = 0; //index for pin[]
boolean new_tag_entry = 0; //in new tag entry mode?
boolean can_add = 0; //if a user is permitted to add other users
int num_users = 500; //number of users the EEPROM can take before it errors out (should debug this)
unsigned long current_time = millis();

void setup()
{  

  Wire.begin();   // start Wire library as I2C-Bus Master
  Serial.begin(115200); 
  RFID_init();
  IO_init();
  printer_init();
  LCD_init();
  // Setup callbacks for SerialCommand commands 
  SCmd.addCommand("ON",SCmd_relay_on);       // Turns relay on
  //format: ON;<relay number(not pin number)>
  SCmd.addCommand("OFF",SCmd_relay_off);        // Turns relay off
  //format: OFF;<relay number(not pin number)> 
  SCmd.addCommand("TOGGLE",SCmd_toggle);  // Toggles relay with duration
  //format: TOGGLE;<relay number(not pin number)>;<time to be on for in a 'delay(arg)' format>
  SCmd.addCommand("RTC",SCmd_rtc);  // uses RTC
  //format: RTC;<0=write, 1=read>;<second, 0-59 (write only)>;<minute,0-59 (write only)>;<hour, 1-23 (write only)>;<dayOfWeek, 1-7 (write only)>;<dayOfMonth, 1-28/29/30/31 (write only)>;<month, 1-12 (write only)>;<year, 0-99 (write only)>
  SCmd.addCommand("DISP",SCmd_display);  // uses display
  //format: DISP;<0=line 1, 1=line 2 (or no argument to clear the display)>;<string (or no argument to clear the line)>
  SCmd.addCommand("PRINT",SCmd_print);  // uses printer
  //format: PRINT;<string>
  SCmd.addCommand("CONTRAST",SCmd_contrast);  // sets contrast
  //format: CONTRAST;<analog value, 0-255 or nothing for default>
  SCmd.addCommand("BACKLIGHT",SCmd_backlight);  // sets backlight
  //format: BACKLIGHT;<analog value, 0-255 or nothing for default>
  SCmd.addCommand("BAT",check_battery);  // reads battery voltage
  //format: BAT
  SCmd.addCommand("TEMP",check_temp);  // reads temperature
  //format: TEMP
  SCmd.addCommand("BEEP",SCmd_beep);  // beeps reader<reader number> for <duration> in delay(arg) format
  //format: BEEP;<reader number>,<duration>
  SCmd.addCommand("READ_CMD",SCmd_read_CMD);  // dumps the command area of the EEPROM
  //format: READ_CMD
  SCmd.addCommand("DUMP_USR",SCmd_dump_users);  // dumps all users up to index
  //format: DUMP_USR
  SCmd.addCommand("ADD_USR",SCmd_add_user);  // adds a user at a given index
  //format: ADD_USR;<index>;<name>;<priv byte>;<hash>
  SCmd.addCommand("READ_USR",SCmd_read_user);  // reads the user at <index>
  //format: READ_USR;<index>
  SCmd.addCommand("RMV_USR",SCmd_remove_user);  // removes the user at <index>
  //format: RMV_USR;<index>
  SCmd.addCommand("RMV_USRN",SCmd_remove_user_name);  // removes the user with <name>
  //format: RMV_USRN;<name>
  SCmd.addCommand("RMV_USRH",SCmd_remove_user_hash);  // removes the user with <hash>
  //format: RMV_USRH;<hash>
  SCmd.addDefaultHandler(unrecognized);  // Handler for command that isn't matched  (says "What?") 

  logging(logDate());
  logging(" - ");
  loggingln("Rebooted"); 
  check_battery(); //log battery at boot
  check_temp(); //log temp at boot
  LCD_default();
  //Serial.println("done");
  //populate_blank_users();
  //write_EE(0,"Evan", 15, "9a7eaa8c16bd8073d32991d7ddc8a480");
  //write_EE(1,"Charlie", 4, "dacbac186709549ff6f16b6a7df0dd1c");
  //write_EE(2,"Mike", 7, "22df7bee944f0b02bc0b9ce8b826e862");
  //write_EE(3,"Whoever", 1, "d2e8102a9b4d8bcd20d38cb1fa2b0ddc");
  //write_EE(4,"Lego", 3, "3bf21d79f6516b061a8326fda21d6af3");
  //blank_EE_cmd();
  //command[0] = 5;
  //write_EE_cmd();
  //dump_all_EE();
  //EE_dump_users();
}


void loop()
{
  //Serial.println(freeRam());
  // Serial.println(reader2Count);
  SCmd.readSerial();     //process serial commands
  current_time = millis();  //saves from calling that function a bunch in the main loop
  if (!digitalRead(exit_button)) //exit button pressed
  {
    //Serial.println("exiting");
    exit();
  }
  //Serial.println("past exiting");

  if (current_time - battery_start > battery_dur) //battery timer loop
  {
    //Serial.println("checking battery");
    check_battery();
  }
  //Serial.println("past checking battery");

  for(int i=0;i<8;i++) //toggle timer loops
  {
    //Serial.println("checking toggled");
    if (toggled[i])
    {
      //Serial.println(millis() - toggle_start[i]);
      if (current_time - toggle_start[i] > toggle_dur[i])
      {
        toggled[i] = 0;
        digitalWrite(relaysToPins[i], LOW);
      }
    }
  }

  if(doorbell) //doorbell timer loop
  {
    if (current_time - doorbell_start > doorbell_dur)
    {
      doorbell = 0;
    }
  }
  //Serial.println("past doorbell");
  if(doorhold) //door hold timer loop
  {
    if (current_time - doorhold_start > doorhold_dur)
    {
      doorhold = 0;
    }
  }
  //Serial.println("past doorhold");
  if (tag_read)
  {
    //Serial.println("tag read");
    if (current_time - RFID_start_time > RFID_time) //RFID time out loop
    {
      //Serial.println("tag timeout");
      LCD_clear();
      LCD_displn("RFID timeout",0);
      RFID_error_out();
    }

    char keypad_button = keypad_matrix();
    if (keypad_button != ' ') //reading a button in tag read mode
    {
      //Serial.println("button pressed");
      if(keypad_button == '*') //escape
      {
        //Serial.println("backspace");
        backspace_pin();
      }
      else if(keypad_button == '#')  //enter
      {
        //Serial.println("enter");
        if (new_tag_entry ==1)
        {
          //Serial.println("new tag");
          new_user_entry();
        }
        else
        {
          //Serial.println("current tag");
          current_user_entry();
        }
      }
      else //other numbers
      {
        //Serial.println("enter pin");
        enter_pin(keypad_button);
      }
    }
  }
  else
  {
    //Serial.println("no tag read");
    char keypad_button = keypad_matrix();
    if (keypad_button != ' ')  //just reading raw buttons, not in tag reading mode
    {
      //Serial.println("button pressed");
      if(keypad_button == '5' && exiting == 1 && can_add == 1)//button press for adding a user
      {
        can_add = 0;
        can_doorbell = 0;
        can_doorhold = 0;
        enter_new_tag_entry_mode();
      }
      if(keypad_button == '2' && exiting == 1 && can_doorbell == 1)//button press for doorbell mode
      {
        can_add = 0;
        can_doorbell = 0;
        can_doorhold = 0;
        if(!doorbell)
        {
          enter_doorbell_mode();
        }
        else
        {
          exit_doorbell_mode();
        }
      }
      if(keypad_button == '3' && exiting == 1 && can_doorhold == 1)//button press for door hold mode
      {
        can_add = 0;
        can_doorbell = 0;
        can_doorhold = 0;
        if(!doorhold)
        {
          enter_doorhold_mode();
        }
        else
        {
          exit_doorhold_mode();
        }
      }
      if(keypad_button == '#' && doorbell == 1)//button press in doorbell mode, combine with below
      {
        exit();
        beep_reader(2,300);
      }
      if(keypad_button == '#' && doorhold == 1)//button press in door hold mode, combine with above
      {
        exit();
        beep_reader(2,300);
      }
      //Serial.print("Button = ");
      //Serial.println(keypad_button);
    }
  }

  if(digit == (sizeof(pin)/sizeof(pin[0]))+1) //too big pin
  {
    logging(logDate());
    logging(" - ");
    loggingln("pin overflow");
    LCD_clear();
    LCD_displn("Error:",0);
    LCD_displn("pin too big",1);
    RFID_error_out();
  }

  if (exiting) 
  {
    if (current_time - exit_start_time > exit_time) //exit time out loop
    {
      exit_timeout();
    }
  }
  if (reader2Count >= 26) //read tag for new input, start tag reading mode (accepting buttons as pin)
  {
    reset_tag_reading(2);
  }
  if (reader1Count >= 26) //read tag for new input, start tag reading mode (accepting buttons as pin)
  {
    reset_tag_reading(1);
  }
  //delay(100);
  //account for reader#Count == 4
}

int freeRam(void)
{
  extern int  __bss_end; 
  extern int  *__brkval; 
  int free_memory; 
  if((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end); 
  }
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval); 
  }
  return free_memory; 
} 

void LCD_init()
{
  Serial3.begin(115200);
  LCD_clear();
    
    transmit = "CONTRAST;";
    transmit += cont_val;
    Serial3.println(transmit);
      
    transmit = "BACKLIGHT;";
    transmit += back_val;
    Serial3.println(transmit);
}


void IO_init()
{
  for (int i=0;i<sizeof(relaysToPins)/sizeof(relaysToPins[0]);i++)
  {
    pinMode(relaysToPins[i], OUTPUT);
  }
  for (int i=0;i<sizeof(analogToPins)/sizeof(analogToPins[0]);i++)
  {
    pinMode(analogToPins[i], INPUT);//not needed?
  }
  pinMode(exit_button,INPUT_PULLUP);//pullup not needed?
}


void printer_init()
{
  for (int i = 0; i < sizeof(dataPins) / sizeof(dataPins[0]); ++i)
    pinMode(dataPins[i], OUTPUT);
  pinMode(strobePin, OUTPUT);
  pinMode(busyPin, INPUT);     
  digitalWrite(strobePin, HIGH);
}


void RFID_init()
{
  pinMode(reader2buzz, OUTPUT);
  pinMode(reader2led, OUTPUT);
  pinMode(reader2led2, OUTPUT);
  digitalWrite(reader2led, 1); //active low
  digitalWrite(reader2led2, 1); //active low
  digitalWrite(reader2buzz, 1); //active low
  attachInterrupt(pinToInterrupt[reader2Pins[1]], callReader2One,  CHANGE);
  attachInterrupt(pinToInterrupt[reader2Pins[0]], callReader2Zero, CHANGE);
  wiegand26.initReaderTwo();


  pinMode(reader1buzz, OUTPUT);
  pinMode(reader1led, OUTPUT);
  pinMode(reader1led2, OUTPUT);
  digitalWrite(reader1led, 1); //active low
  digitalWrite(reader1led2, 1); //active low
  digitalWrite(reader1buzz, 1); //active low
  attachInterrupt(pinToInterrupt[reader1Pins[1]], callReader1One,  CHANGE);
  attachInterrupt(pinToInterrupt[reader1Pins[0]], callReader1Zero, CHANGE);
  wiegand26.initReaderOne();
}


// logs temperature
void check_temp()
{
  char temp[8];
  current_temperature = getTemp();//stored for calculation of contrast value later
  ftoa(temp,current_temperature,2);
  //dtostrf(voltage,8,2,volt); should work?
  printname = "Temperature outside: ";
  printname += temp;

  logging(logDate());
  logging(" - ");
  loggingln(printname);
}


float getTemp()
{
  byte data[9];
  byte addr[8];
  int temp;

  while ( !ds.search(addr)) 
  {
    //Serial.print("No more addresses.\n");
    ds.reset_search();
  }
  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);
  delay(600);
  byte present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);
  for (byte i = 0; i < 9; i++) 
  {
    data[i] = ds.read();
  }
  boolean negative = (data[1] & 1<<5);
  data[1] <<=4;
  data[1] >>=4;
  temp = data[1];
  temp <<= 8;             /* shift left eight bits */
  temp += data[0];        /* add second byte of temperature */
  float tempFloat = (float)temp/16.0;
  if(negative)
  {
    tempFloat = -tempFloat;
  }
  return tempFloat;
}


char keypad_matrix()
{
  //Serial.println("keypad_matrix");
  if(Serial3.available())
  {
    char result = Serial3.read();//Read key press
    return result;
  }
  else
  {
    return ' ';//equivalent to -1, a null result
  }
  //delay(10);
}


void LCD_default()
{
  LCD_clear();
  if(doorbell)
  {
    LCD_displn("Welcome: press  ",0);
    LCD_displn("\"#\" to enter    ",1);
  }
  else if(doorhold)
  {
    LCD_displn("Door held: press",0);
    LCD_displn("\"#\" to enter    ",1);
  }
  else
  {
    LCD_displn("i3 Detroit      ",0);
    LCD_displn("Hackerspace     ",1);
  }
}

void exit_timeout()
{
  logging(logDate());
  logging(" - ");
  loggingln("exit relay closed");
  LCD_default();
  RFID_reset();
  exiting = 0; //no longer in exiting mode
  //exit_start_time = 0; //not needed
  digitalWrite(relaysToPins[exit_relay], LOW); //close door
}

// reads battery voltage
void check_battery()
{
  battery_start = millis();
  float voltage = ((float)(analogRead(A15)*20)/1024.0);
  if(voltage<voltage_threshold)
  {
    battery_dur = (1000*60*5); //set to 2 mins
    logging(logDate());
    logging(" - ");
    loggingln("battery voltage dropped below threshold");
  }
  else
  {
    battery_dur = (1000*60*15); //set to 1 hour
  }

  char volt[8];
  ftoa(volt,voltage,2);
  //dtostrf(voltage,8,2,volt); should work?
  printname = "Battery voltage: ";
  printname += volt;

  logging(logDate());
  logging(" - ");
  loggingln(printname);
}


char *ftoa(char *a, double f, int precision)
{
  long p[] = {
    0,10,100,1000,10000,100000,1000000,10000000,100000000  };

  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long decimal = abs((long)((f - heiltal) * p[precision]));
  itoa(decimal, a, 10);
  return ret;
}


void exit_doorbell_mode()
{
  logging(logDate());
  logging(" - ");
  loggingln("exit doorbell mode");
  LCD_clear();
  LCD_displn("doorbell mode",0);
  LCD_displn("deactivated",1);
  doorbell = 0;
  //doorbell_start = 0;
  //can_doorbell = 0;
}


void enter_doorbell_mode()
{
  logging(logDate());
  logging(" - ");
  loggingln("enter doorbell mode");
  LCD_clear();
  LCD_displn("doorbell mode",0);
  LCD_displn("activated",1);
  doorbell =1;
  doorbell_start = current_time;
  //can_doorbell = 0;
}


void exit_doorhold_mode()
{
  logging(logDate());
  logging(" - ");
  loggingln("exit door hold mode");
  LCD_clear();
  LCD_displn("door hold mode",0);
  LCD_displn("deactivated",1);
  doorhold = 0;
  //doorhold_start = 0;
  //can_doorhold = 0;
}


void enter_doorhold_mode()
{
  logging(logDate());
  logging(" - ");
  loggingln("enter door hold mode");
  LCD_clear();
  LCD_displn("door hold mode",0);
  LCD_displn("activated",1);
  doorhold =1;
  doorhold_start = current_time;
  //can_doorhold = 0;
}


void enter_new_tag_entry_mode()
{
  exit_timeout();
  logging(logDate());
  logging(" - ");
  loggingln("new member attempt");
  LCD_clear();
  LCD_displn("New member:",0);
  LCD_displn("swipe new tag",1);
  new_tag_entry = 1;
}


void exit()
{
  logging(logDate());
  logging(" - ");
  loggingln("exit relay open");
  exiting = 1;
  exit_start_time = current_time;
  digitalWrite(relaysToPins[exit_relay], HIGH);
}


void new_user_entry()
{
  //Serial.print("Tag = ");
  //Serial.println(tag,DEC);
  //Serial.print("Pin = ");
  //for(int i=0;i<digit;i++)
  //{
  //	Serial.print(pin[i]);      
  //}
  if (digit<4) //too short of a pin
  {
    clear_pin();
    int d = 300;
    LCD_clear();
    LCD_displn("pin too short",0);
    beep_repeat(2,200,2);
    reader2 = tag;//hack
    reset_tag_reading(2);//hack
    RFID_start_time = millis();
  }
  else
  {
    String toHash = "";
    toHash += tag;
    //put in raw tag to eeprom
    for(int i=0;i<(sizeof(pin)/sizeof(pin[0]));i++)
    {
      toHash += pin[i];   
    }
    clear_pin();
    char hashed[32];
    toHash.toCharArray(hashed,32); 
    char *md5str = MD5::make_digest(MD5::make_hash(hashed), 16);
    for (int i=0;i<num_users;i++)
    {
      if(!user_present(i))
      {
        String name = "New Member ";
        name += logDate();
        priv= 1;
        //put in raw tag to eeprom
        write_EE(i, name, priv, md5str);
        break;
      }
    }
    //Serial.println(md5str);
    logging(logDate());
    logging(" - ");
    logging("new user ");
    logging(md5str);
    loggingln(" added");
    LCD_clear();
    LCD_displn("Welcome",0);
    LCD_displn("New member",1);

    for(int i=0;i<(sizeof(hashed)/sizeof(hashed[0]));i++) //also should not be needed
    {
      hashed[i] = 0;   
    }
    new_tag_entry = 0;
    toHash = ""; //not needed?
    RFID_reset();
    exit();
  }
}

void current_user_entry()
{
  //possibly add a time based lack of pin requirement, for example 
  //store a list of user IDs that have no pin entry allowed, reset 
  //it at midnight (or noon) and check the card id against all of those before hashing, etc...
  
  //Serial.print("Tag = ");
  //Serial.println(tag,DEC);
  //Serial.print("Pin = ");
  //for(int i=0;i<digit;i++)
  //{
  //	Serial.print(pin[i]);      
  //}
  String toHash;
  toHash += tag;
  for(int i=0;i<(sizeof(pin)/sizeof(pin[0]));i++)
  {
    toHash += pin[i];   
  }
  clear_pin();
  char hashed[32];
  toHash.toCharArray(hashed,32); 
  char *md5str = MD5::make_digest(MD5::make_hash(hashed), 16);
  if(authenticate(md5str) == 1)
  {
    logging(logDate());
    logging(" - ");
    printname = "";
    for (int j = 0; j < (sizeof(name)/sizeof(name[0])); j++)
    {
      if(name[j] != 0)
      {
        printname += name[j];
        //Serial.println(name[j],HEX);
      }
    }
    logging(printname);
    loggingln(" carded in");
    LCD_clear();
    LCD_displn("Welcome Member:",0);
    LCD_displn(printname,1);
    exit();
  }
  else
  {
    logging(logDate());
    logging(" - ");
    //logging(md5str);//
    //logging(" - ");//
    loggingln("card entry failed");
    LCD_clear();
    LCD_displn("Invalid entry",0);
    RFID_error_out();
  }
  //Serial.println(md5str);
  //Serial.println();
  //Serial.println(test);
  for(int i=0;i<(sizeof(hashed)/sizeof(hashed[0]));i++)//should not be needed
  {
    hashed[i] = 0;   
  }
  toHash = ""; //also not needed?
  RFID_reset();
}



void backspace_pin()
{
  pin[digit] = 15;
  //Serial.println("backspace");
  LCD_displn("                ",1);
  LCD_displn("Pin:",1);
  if(digit >0)
  {
    digit--;
    pin[digit] = 15;
  }
  for (int i = 0;i<digit;i++)//repopulates asterisks
  {
    LCD_disp("*");
  }
  //for(int i=0;i<(sizeof(pin)/sizeof(pin[0]));i++)
  //{
  //	Serial.print(pin[i]);      
  //}
}


void enter_pin(char c)
{
  //Serial.print("Pin Button = ");
  //Serial.println(reader2);
  LCD_disp("*");
  pin[digit] = c - '0';//stores as numbers rather than characters
  digit++;
}


void reset_tag_reading(int reader_num)
{
  //Serial.print("tag reader ");
  //Serial.println(reader_num,HEX);
  if(reader_num == 1)
  {
    reader1 = reader1 >> 1;//fix for extra zero
    tag = reader1;
    reader1Count = 0;
    reader1 = 0;
  }
  else if(reader_num == 2)
  {
    reader2 = reader2 >> 1;//fix for extra zero
    tag = reader2;
    reader2Count = 0;
    reader2 = 0;	
  }
  printname = "";
  printname += tag;
  logging(logDate());
  logging(" - ");
  logging(printname);
  loggingln(" tag read");
  LCD_clear();
  LCD_displn("Please Enter",0);
  LCD_displn("Pin:",1);
  tag_read = 1;
  clear_pin();//not needed?
  RFID_start_time = current_time;
}


void write_EE(int index, String name, byte priv, String hash)
{
  index +=1;
  char data[64] = {
  };
  for(int i = 0;i<31;i++)
  {
    if(name[i] == 0)
    {
      i+=31;//break
    }
    else
    {
      data[i] = name[i];
    }
  }
  data[31] = priv;
  for(int i = 0;i<32;i++)
  {
    data[i+32] = hash[i];
  }

  int start_address = (index) * entry_size;
  int end_address = start_address + entry_size;

  //Serial.println("Writing data");
  for (int address = start_address; address < end_address; address++)
  {
    EEPROM1024.write(address, data[address % entry_size]);
  }
  //Serial.println();
}


void dump_all_EE() //debug stuff
{
  for (int address = 0; address < 131072; address++)
  {
    char data;
    data = EEPROM1024.read(address);
    Serial.print(address);
    Serial.print(" :");
    Serial.println(data, HEX);
    Serial.print(address);
    Serial.print(" :");
    Serial.println((char)data);
    //Serial.print(" :");
    //Serial.println(data);
  }
  //Serial.println("DONE");
}


// dumps all users present
//format: DUMP_USR
void SCmd_dump_users()
{
  for (int i=0;i<num_users;i++)
  {
    if(user_present(i))
    {
      Serial.println(i);
      serial_user_print(i);
    }
  }
  //Serial.println("DONE");
}


// reads the user at <index>
//format: READ_USR;<index>
void SCmd_read_user()
{
  char *arg; 
  arg = SCmd.next();
  serial_user_print(atoi(arg));
  //Serial.println("DONE");
}


//create a function like SCmd_add_user that adds a user at the first free index


// adds a user at a given index
//format: ADD_USR;<index>;<name>;<priv byte>;<hash>
void SCmd_add_user()//rename to SCmd_add_user_index() once above function is done
{
  char *arg; 
  arg = SCmd.next();
  int index = atoi(arg);
  arg = SCmd.next();
  String name = arg;
  arg = SCmd.next();
  byte priv= atoi(arg);
  arg = SCmd.next();
  String hash = arg;
  write_EE(index, name, priv, hash);
}


void populate_blank_users()//debug
{
  for (int i = 0;i<num_users;i++)
    remove_user(i);
}


// removes the user at <index>
//format: RMV_USR;<index>
void SCmd_remove_user()
{
  char *arg; 
  arg = SCmd.next(); 
  remove_user(atoi(arg));
  Serial.println("user removed");
}


void remove_user(int i)
{
  logging(logDate());
  logging(" - ");
  loggingln("removing user");
  serial_user_print(i);
  String name = "Free";
  byte priv= 0;
  //gonna need to add a blank cardid ince that gets implemented
  String hash = "00000000000000000000000000000000";
  write_EE(i, name, priv, hash);
  logging(logDate());
  logging(" - ");
  loggingln("user removed");
}


// removes the user with <name>
//format: RMV_USRN;<name>
void SCmd_remove_user_name()
{
  char *arg; 
  arg = SCmd.next(); 
  remove_user_name(arg);
  Serial.println("user removed");
}


void remove_user_name(char named[31])///////////////////////////////////////////////////////////////////////////////////////////////untested
{
  for(int i = 0;i<num_users;i++)
  {
    boolean succeed = 1;
    read_EE_name(i);
    for (int j = 0; j < (sizeof(name)/sizeof(name[0])); j++)
    {
      if((name[j] != 0 || named[j] != 0) && name[j] != named[j])
      {
        succeed == 0;
        j+=((sizeof(name)/sizeof(name[0])));//break
      }
    }
    if(succeed)
    {
      remove_user(i);
      Serial.println("user removed");
      i+=num_users;//break
    }
  }
}


//gonna need to be able to remove jusers based on cardid once that gets implemented


// removes the user with <hash>
//format: RMV_USRH;<hash>
void SCmd_remove_user_hash()
{
  char *arg; 
  arg = SCmd.next(); 
  remove_user_hash(arg);
  Serial.println("user removed");
}


void remove_user_hash(char hashd[32])////////////////////////////////////////////////////////////////////////////////////////////////untested
{
  for(int i = 0;i<num_users;i++)
  {
    boolean succeed = 1;
    read_EE_hash(i);
    for (int j = 0; j < (sizeof(hash)/sizeof(hash[0])); j++)
    {
      if(hash[j] != hashd[j])
      {
        succeed == 0;
        j+=((sizeof(hash)/sizeof(hash[0])));//break
      }
    }
    if(succeed)
    {
      remove_user(i);
      Serial.println("user removed");
      i+=num_users;//break
    }
  }
}


void serial_user_print(int index)//debug? if in final make serial prints logging
{
  read_EE_name(index);
  read_EE_priv(index);
  read_EE_hash(index);
  Serial.print(index,DEC);
  Serial.print(";");
  for (int j = 0; j < (sizeof(name)/sizeof(name[0])); j++)
  {
    if(name[j] != 0)
      Serial.print(name[j]);
  }
  Serial.print(";");
  Serial.print(priv,HEX);
  Serial.print(";");
  for  (int j = 0; j < (sizeof(hash)/sizeof(hash[0])); j++)
  {
    Serial.print((char)hash[j]);
  }
  Serial.println();
}


void dump_EE(int start_address, int end_address)//debug
{
  //byte data[64];
  for (int address = start_address; address < end_address; address++)
  {
    char data;
    data = EEPROM1024.read(address);
    Serial.print(address);
    Serial.print(" :");
    Serial.print(data, HEX);
    Serial.print(" :");
    Serial.println(data);
  }
  //Serial.println("DONE");
  return;
}


void read_EE_hash(int index)
{
  //for(int i = 0;i<64;i++)
  //{
  //	EEPROM1024.write(i,command[i]);
  //}
  index +=1;//keeps command byte unwritten
  int start_address = (index) * entry_size;
  int end_address = start_address + entry_size;

  for (int i = 0; i < 32; i++)
  {
    hash[i] = EEPROM1024.read(((start_address) + 32)+i);
  }
  return;
}


void read_EE_priv(int index)
{
  priv = EEPROM1024.read(((index+1) * entry_size) + 31);//keeps command byte unwritten
  return;
}


void read_EE_name(int index)
{
  for(int i = 0;i<31;i++)
  {
    name[i] = 0;
  }
  for (int i = 0; i < 31; i++)
  {
    name[i] = EEPROM1024.read(i+((index+1) * entry_size));//keeps command byte unwritten
  }
  return;
}


void read_EE_command()//populate working memory from EEPROM
{
  //for(int i = 0;i<64;i++)
  //{
  //	command[i] = 0;
  //}
  for(int i = 0;i<64;i++)
  {
    command[i] = EEPROM1024.read(i);
  }
}


void write_EE_cmd()//populate EEPROM from working memory
{
  for(int i = 0;i<64;i++)
  {
    EEPROM1024.write(i,command[i]);
  }
}


void blank_EE_cmd()//debug
{
  for(int i = 0;i<64;i++)
  {
    command[i] = 0;
    EEPROM1024.write(i,0);
  }
}


boolean user_present(int i)
{
  read_EE_priv(i);
  if(priv != 0)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}


boolean can_enter(int i)
{
  read_EE_priv(i);
  if((priv & bit(PRIV_ACCESS)) == bit(PRIV_ACCESS))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}


boolean authenticate(char *md5str)
{

  byte hard_hash[5][33] = {
    {/*"e0bdc8c7b2e2d33e7889858a30aad569"*/
    }
    ,{
    }
    ,{
    }
    ,{
    }
    ,{
    }
  };/////////5 hashes
  boolean succeed = 1;
  for (int k=0;k<5;k++)
  {
    for (int j=0;j<32;j++)
    {
      if(hard_hash[k][j] != md5str[j])
      {
        //Serial.print("fail: ");
        //Serial.println(k,DEC);
        //Serial.println(j,DEC);
        //Serial.println(hash[j],HEX);
        //Serial.println((md5str[j]),HEX);
        succeed = 0;
        j+=32;
      }
    }
    if(succeed)
    {
      printname = "Failsafe";
      for (int j = 0; j < printname.length(); j++)///////////////////////////////////////////////////////////////////////untested
      {
        if(printname[j] != 0)
        {
          name[j] = printname[j];
          //Serial.println(name[j],HEX);
        }
      }



      logging(logDate());
      logging(" - ");
      loggingln("Failsafe used");
      return 1;
    }
  }

  for (int i=0;i<num_users;i++)
  {
    if(can_enter(i))
    {
      read_EE_hash(i);
      succeed = 1;
      for (int j=0;j<32;j++)
      {
        if(hash[j] != (md5str[j]))
        {
          //Serial.print("fail: ");
          //Serial.println(i,DEC);
          //Serial.println(j,DEC);
          //Serial.println(hash[j],HEX);
          //Serial.println((md5str[j]),HEX);
          succeed = 0;
          break;
        }
      }
      if(succeed == 1)
      {
        read_EE_name(i);
        read_EE_priv(i);
        read_EE_hash(i);
        //Serial.print("win: ");
        //serial_user_print(i);
        if((priv & bit(PRIV_ADDUSER)) == bit(PRIV_ADDUSER))
        {
          can_add = 1;
          //Serial.println("can add");
        }
        if((priv & bit(PRIV_DOORBELL)) == bit(PRIV_DOORBELL))
        {
          can_doorbell = 1;
          //Serial.println("can doorbell");
        }
        if((priv & bit(PRIV_DOORHOLD)) == bit(PRIV_DOORHOLD))
        {
          can_doorhold = 1;
          //Serial.println("can door hold");
        }
        return 1;
      }
    }
  }
  return 0;
}


void RFID_reset()
{
  clear_pin();
  //RFID_start_time = 0; //not needed
  tag_read = 0;
  digit = 0;
  new_tag_entry = 0;
}


void RFID_error_out()
{
  can_add = 0; //reset user capabilities
  can_doorhold = 0; //reset user capabilities
  can_doorbell = 0; //reset user capabilities
  RFID_reset();
  beep_repeat(2,200,3);//delay timer
  LCD_default();
}


void beep_repeat(int reader,int time,int number)//blocking
{
  /*Serial.print("beep repeat");
  Serial.print(" ");
  Serial.print(reader);
  Serial.print(" ");
  Serial.print(time);
  Serial.print(" ");
  Serial.println(number);*/
  for(int i = 0;i<number;i++)
  {
    beep_reader(reader,time);
    delay(time);
  }
}


void beep_reader(int reader,int d)//blocking
{
  switch(reader){
  case 0:
    {
      digitalWrite(reader2led, 0);
      digitalWrite(reader2buzz, 0);
      digitalWrite(reader1led, 0);
      digitalWrite(reader1buzz, 0);
      delay(d);
      digitalWrite(reader2led, 1);
      digitalWrite(reader2buzz, 1);
      digitalWrite(reader1led, 1);
      digitalWrite(reader1buzz, 1);
    }
  case 1:
    {
      digitalWrite(reader1led, 0);
      digitalWrite(reader1buzz, 0);
      delay(d);
      digitalWrite(reader1led, 1);
      digitalWrite(reader1buzz, 1);
    }
  case 2:
    {
      digitalWrite(reader2led, 0);
      digitalWrite(reader2buzz, 0);
      delay(d);
      digitalWrite(reader2led, 1);
      digitalWrite(reader2buzz, 1);
    }
  default:
    {
    }
  }
}


// dumps the command area of the EEPROM
void SCmd_read_CMD()//debug, dumps command memory
{
  read_EE_command();//updates working memory from EEPROM
  for  (int i = 0; i < 64; i++)
  {
    Serial.print(command[i], HEX);
  }
  Serial.println();
}


void clear_pin()
{
  digit = 0;
  for(int i=0;i<(sizeof(pin)/sizeof(pin[0]));i++)
  {
    pin[i] = 15;     
  }
}


// Turns relay on
//format: ON;<relay number(not pin number)> 
void SCmd_relay_on()
{
  char *arg; 
  arg = SCmd.next(); 
  if (arg != NULL && atoi(arg) < 8 && ((atoi(arg) == exit_relay && exiting == 1) ? 0 : 1)) //protect exit relay
  {
    digitalWrite(relaysToPins[atoi(arg)],HIGH); 
  }
}


// Turns relay off
//format: OFF;<relay number(not pin number)> 
void SCmd_relay_off()
{
  char *arg; 
  arg = SCmd.next(); 
  if (arg != NULL && atoi(arg) < 8 && ((atoi(arg) == exit_relay && exiting == 1) ? 0 : 1))  //protect exit relay
  {
    digitalWrite(relaysToPins[atoi(arg)],LOW); 
  }
}


// beeps reader<reader number> for <duration> in delay(arg) format
//format: BEEP;<reader number>;<duration>
void SCmd_beep()
{
  Serial.println("beepcommand");
  char *arg; 
  arg = SCmd.next(); 
  if (arg != NULL) 
  {
    if(atoi(arg) == 1)
    {
      arg = SCmd.next(); 
      if (arg != NULL) 
      {
        beep_reader(1,atoi(arg));
      }
    }
    else if(atoi(arg) == 2)
    {
      arg = SCmd.next(); 
      if (arg != NULL) 
      {
        beep_reader(2,atoi(arg));
      }
    }
    else if(atoi(arg) == 0)
    {
      arg = SCmd.next(); 
      if (arg != NULL) 
      {
        beep_reader(0,atoi(arg));
      }
    }
  }
}


// Toggles relay with duration
//format: TOGGLE;<relay number(not pin number)>;<time to be on for in a 'delay(arg)' format>
void SCmd_toggle()    
{
  int relay; 
  int dur; 
  char *arg; 
  //Serial.println("toggle"); 
  arg = SCmd.next(); 
  if (arg != NULL && atoi(arg) < 8 && ((atoi(arg) == exit_relay && exiting == 1) ? 0 : 1)) 
  {
    relay=atoi(arg);    // Converts a char string to an integer
    //Serial.print("relay no: "); 
    //Serial.println(relay); 
  } 
  else 
  {
    //Serial.println("No arguments"); 
  }
  arg = SCmd.next(); 
  if (arg != NULL) 
  {
    dur=atoi(arg); 
    //Serial.print("Duration: "); 
    //Serial.println(dur); 
    digitalWrite(relaysToPins[relay], HIGH);
    toggled[relay] = 1;
    toggle_dur[relay] = dur;
    toggle_start[relay] = millis();
  } 
  else 
  {
    //Serial.println("No second argument"); 
  }
}


// uses RTC
//format: RTC;<0=write, 1=read>;<second, 0-59 (write only)>;<minute,0-59 (write only)>;<hour, 1-23 (write only)>;<dayOfWeek, 1-7 (write only)>;<dayOfMonth, 1-28/29/30/31 (write only)>;<month, 1-12 (write only)>;<year, 0-99 (write only)>
void SCmd_rtc()    
{
  char *arg; 
  //Serial.println("rtc"); 
  arg = SCmd.next(); 
  if (arg != NULL) 
  {
    if(atoi(arg))
    {
      //Serial.println("read rtc"); 
      //logDate();
      Serial.println(logDate());//should be logging?
    } 
    else
    {
      //Serial.println("write rtc"); 
      arg = SCmd.next(); 
      if (arg != NULL) 
      {
        second=atoi(arg); 
      } 
      else 
      {
        //Serial.println("No second argument"); 
      }
      arg = SCmd.next(); 
      if (arg != NULL) 
      {
        minute=atoi(arg); 
      } 
      else 
      {
        //Serial.println("No minute argument"); 
      }
      arg = SCmd.next(); 
      if (arg != NULL) 
      {
        hour=atoi(arg); 
      } 
      else 
      {
        //Serial.println("No hour argument"); 
      }
      arg = SCmd.next(); 
      if (arg != NULL) 
      {
        dayOfWeek=atoi(arg); 
      } 
      else 
      {
        //Serial.println("No dow argument"); 
      }
      arg = SCmd.next(); 
      if (arg != NULL) 
      {
        dayOfMonth=atoi(arg); 
      } 
      else 
      {
        //Serial.println("No dom argument"); 
      }
      arg = SCmd.next(); 
      if (arg != NULL) 
      {
        month=atoi(arg); 
      } 
      else 
      {
        //Serial.println("No month argument"); 
      }
      arg = SCmd.next(); 
      if (arg != NULL) 
      {
        year=atoi(arg); 
      } 
      else 
      {
        //Serial.println("No year argument"); 
      }     
      ds1307.setDateDs1307(second,minute,hour,dayOfWeek,dayOfMonth,month,year);         
      /*  Sets the date/time (needed once at commissioning)
       			uint8_t second       // 0-59
       			uint8_t minute       // 0-59
       			uint8_t hour         // 1-23
       			uint8_t dayOfWeek    // 1-7
       			uint8_t dayOfMonth   // 1-28/29/30/31
       			uint8_t month        // 1-12
       			uint8_t year         // 0-99
       			*/
    }
  }
  else 
  {
    //Serial.println("No arguments"); 
  }
}


void LCD_displn(String prints,int line)//displays on a given line, does not emulate CR LF
{
    transmit = "DISPLN;";
    transmit += line;
    transmit += ";";
    transmit += prints;
    Serial3.println(transmit);
}


void LCD_disp(String prints)
{
    transmit = "DISP;";
    transmit += prints;
    Serial3.println(transmit);
}


inline void LCD_clear()
{
  LCD_displn("                ",0);
  LCD_displn("                ",1);
}


// uses display
//format: DISP;<0=line 1, 1=line 2 (or no argument to clear the display)>;<string (or no argument to clear the line)>
void SCmd_display()    
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


// sets contrast
//format: CONTRAST;<analog value, 0-255 or nothing for default>
void SCmd_contrast()    
{
  
    transmit = "CONTRAST;";
  char *arg; 
  //Serial.println("contrast"); 
  arg = SCmd.next(); 
  if (arg != NULL) 
  {
    transmit += arg;
  }
  else
  {
    transmit += cont_val;
  }
    Serial3.println(transmit);
}


// sets backlight
//format: BACKLIGHT;<analog value, 0-255 or nothing for default>
void SCmd_backlight()    
{
  
    transmit = "BACKLIGHT;";
  char *arg; 
  //Serial.println("backlight"); 
  arg = SCmd.next(); 
  if (arg != NULL) 
  {
    transmit += arg;
  }
  else
  {
    transmit += back_val;
  }
    Serial3.println(transmit);
}


// uses printer
//format: PRINT;<string>
void SCmd_print()    
{
  char *arg; 
  //Serial.println("print"); 
  arg = SCmd.next(); 
  if (arg != NULL) 
  {
    printerln(arg);
  }
}


void printer(String printable)    
{
  //if (printable.length() != 0) 
  //{
  for(int i=0;i<printable.length();i++)//change to not call .length every time
  {
    printing(printable[i]);
  }
  //} 
  //else {
  //Serial.println("No arguments"); 
  //}
}


void printerln(String printable)    
{
  printer(printable);
  printing(0x0d);//CR
  printing(0x0a);//LF
} 


void logging(String tolog)//wrapper
{
  Serial.print(tolog);
  printer(tolog);
}


void loggingln(String tolog)//wrapper
{
  Serial.println(tolog);
  printerln(tolog);
}


void printing(char buffer)//blocking? only if BusyPin is ever high?
{ 
  //Write out our buffer
  for (int j = 0; j < sizeof(dataPins) / sizeof(dataPins[0]); ++j)
    digitalWrite(dataPins[j], (buffer & (1 << j)));
  //buffer = 0x00;

  //Block and wait for the printer to be ready
  while (digitalRead(busyPin));

  //Tell the printer to write out the data
  digitalWrite(strobePin, LOW);
  delay(10);
  digitalWrite(strobePin, HIGH);
  delay(10);//not needed?
  //Reset the data lines
  for (int j = 0; j < sizeof(dataPins) / sizeof(dataPins[0]); ++j) 
    digitalWrite(dataPins[j], LOW);
}


String logDate()
{
  String date;
  ds1307.getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  date+="20";//hack
  date+=year;
  date+="-";
  if(month < 10)
  {
    date+="0";
  }
  date+=month;
  date+="-";
  if(dayOfMonth < 10)
  {
    date+="0";
  }
  date+=dayOfMonth;
  date+='T';
  if(hour < 10)
  {
    date+="0";
  }
  date+=hour;
  date+=":";
  if(minute < 10)
  {
    date+="0";
  }
  date+=minute;
  date+=":";
  if(second < 10)
  {
    date+="0";
  }
  date+=second;
  date+=" ";
  switch(dayOfWeek){
  case 1:
    {
      date+="SUN";
      break;
    }
  case 2:
    {
      date+="MON";
      break;
    }
  case 3:
    {
      date+="TUE";
      break;
    }
  case 4:
    {
      date+="WED";
      break;
    }
  case 5:
    {
      date+="THU";
      break;
    }
  case 6:
    {
      date+="FRI";
      break;
    }
  case 7:
    {
      date+="SAT";
      break;
    }  
  }
  return date;
}


// This gets set as the default handler, and gets called when no other command matches. 
void unrecognized()
{
  Serial.println("What?"); 
}


void callReader1Zero(){
  wiegand26.reader1Zero();
}
void callReader1One(){
  wiegand26.reader1One();
}
void callReader2Zero(){
  wiegand26.reader2Zero();
}
void callReader2One(){
  wiegand26.reader2One();
}

