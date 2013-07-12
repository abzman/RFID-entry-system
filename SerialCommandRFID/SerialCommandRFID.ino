// RFID project
// i3 Detroit
// Evan Allen
// June 2013


#include <MD5.h>


//library to allow it to take commands from serial
#include <SerialCommand.h>
SerialCommand SCmd;   // The demo SerialCommand object

#include <OneWire.h>
/* Interface to DS1822 chip, record data to eeprom */
OneWire ds(30);  /* define the pin the interface is on */

//I2C stuff
#include <Wire.h>         // Needed for I2C Connection to the DS1307 date/time chip
#include <DS1307.h>       // DS1307 RTC Clock/Date/Time chip library
#include <E24C1024.h>     // Needed for the onboard EEprom
uint8_t second, minute, hour, dayOfWeek, dayOfMonth, month, year;     // Global RTC clock variables. Can be set using DS1307.getDate function.
DS1307 ds1307;        // RTC Instance


//RFID reader stuff
#include <WIEGAND26.h>    // Wiegand 26 reader format libary
byte reader1Pins[]={19,18};          // Reader 1 connected to pins 19,18
byte reader2Pins[]={2,3};          // Reader2 connected to pins 2,3
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



//Get the interrupt number of a given digital pin on an arduino mega.
const byte pinToInterrupt[22] = {-1,-1,0,1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,5,4,3,2};



//pin-mappings
const int relaysToPins[8] = {31,32,33,34,35,36,37,38};
const int analogToPins[16] = {54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69};
const int disallowedOutputs[34] = {0,1,2,3,4,5,6,7,8,9,14,15,16,17,18,19,20,21,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69}; //serial, i2c, rs485, wiegand pins, analog inputs
const int disallowedDigitalInputs[41] = {0,1,2,3,4,5,6,7,8,14,15,16,17,18,19,20,21,31,32,33,34,35,36,37,38,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69}; //serial, i2c, rs485, relays,wiegand pins, analog inputs


//exit button stuff
const int exit_button = 48; //digital pinthe exit button is attached to
const int exit_relay = 0; //relay the door solenoid is on
const unsigned long exit_time = 3000; //time to keep the door open in thousandths of a second
boolean exiting = 0; //wether or not the door should be open (and timer running)
unsigned long exit_start_time = 0; //used for count down timer


//RFID timer stuff
const unsigned long RFID_time = 30000; //timeout for the RFID scan in thousandths of a second
unsigned long RFID_start_time = 0; //used for the count down timer



//dot matrix printer stuff
const int dataPins[] = {41,40,43,42,45,44,47,46};
const int strobePin = 39;//active low
const int busyPin = 49;//active low



//EEPROM stuff
//if I have time I should figure out why it errors out reading over 512 entries
int entry_size = 64;
byte hash[32];
char name[31];
byte priv;
byte command[64]; //add time zone


//doorbell mode stuff
boolean doorbell = 0; //in doorbell mode?
boolean can_doorbell = 0; //can a given user enter doorbell mode?
unsigned long doorbell_start = 0; //used for count down timer
unsigned long doorbell_dur = (3600000*12); //timeout for doorbell in hours

//toggle stuff
//unsigned long max_toggle = 15000; //maximum amount of time a toggle can sieze up the processor with a delay loop
unsigned long toggle_dur[8] = {0,0,0,0,0,0,0,0}; //time in thousandths of a second
unsigned long toggle_start[8] = {0,0,0,0,0,0,0,0}; //start time
boolean toggled[8] = {0,0,0,0,0,0,0,0}; //active or not


//door hold mode stuff
boolean doorhold = 0; //in door hold mode?
boolean can_doorhold = 0; //can a given user enter door hold mode?
unsigned long doorhold_start = 0; //used for count down timer
unsigned long doorhold_dur = (60000*10); //timeout for door hold in minutes


//battery stuff
long voltage_threshold = 12.5; //threshold to throw an error at
unsigned long battery_start = 0; //used for count down timer
unsigned long battery_dur = (3600000); //time between battery checks (1 hour default)

//LCD stuff
#include <I2CLCD.h>
int contrast = 9;
int cont_value = 85;
I2CLCD lcd = I2CLCD(0x12, 16, 2); //set for 16X2 LCD

//keypad stuff
char last = ' ';

long tag = 0; //stores tag
boolean tag_read = 0; //in pin readingmode
byte pin[8] = {15,15,15,15,15,15,15,15}; //untypable character, unlike 0
byte digit = 0; //index for pin[]
boolean new_tag_entry = 0; //in new tag entry mode?
boolean can_add = 0; //if a user ispermitted to add other users
int num_users = 500; //number of users the EEPROM can take before it errors out (should debug this


void setup()
{  

	Wire.begin();   // start Wire library as I2C-Bus Master
	Serial.begin(9600); 
	RFID_init();
	IO_init();
	printer_init();
	keypad_init();
	battery_start = millis(); //start battery timer
	LCD_init();
	// Setup callbacks for SerialCommand commands 
	SCmd.addCommand("ON",relay_on);       // Turns relay on
	//format: ON;<relay number(not pin number)>
	SCmd.addCommand("OFF",relay_off);        // Turns relay off
	//format: OFF;<relay number(not pin number)> 
	SCmd.addCommand("TOGGLE",toggle_command);  // Toggles relay with duration
	//format: TOGGLE;<relay number(not pin number)>;<time to be on for in a 'delay(arg)' format>
	SCmd.addCommand("READ",read_command);  // reads data
	//format: READ;<0=analog, 1=digital>;<pin number (digital pin # for digital and analog pin A# for analog>
	SCmd.addCommand("RTC",rtc_command);  // uses RTC
	//format: RTC;<0=write, 1=read>;<second, 0-59 (write only)>;<minute,0-59 (write only)>;<hour, 1-23 (write only)>;<dayOfWeek, 1-7 (write only)>;<dayOfMonth, 1-28/29/30/31 (write only)>;<month, 1-12 (write only)>;<year, 0-99 (write only)>
	SCmd.addCommand("DISP",display_command);  // uses display
	//format: DISP;<0=line 1, 1=line 2 (or no argument to clear the display)>;<string (or no argument to clear the line)>
	SCmd.addCommand("PRINT",print_command);  // uses printer
	//format: PRINT;<string>
	SCmd.addCommand("BAT",battery_command);  // reads battery voltage
	//format: BAT
	SCmd.addCommand("TEMP",temp_command);  // reads battery voltage
	//format: TEMP
	SCmd.addCommand("BEEP",beep_command);  // beeps reader<reader number> for <duration> in delay(arg) format
	//format: BEEP;<reader number>,<duration>
	SCmd.addCommand("EE_READ_CMD",EE_read_CMD_command);  // dumps the command area of the eeprom
	//format: EE_READ_CMD
	SCmd.addCommand("EE_DUMP_USR",EE_dump_users);  // dumps all users up to index
	//format: EE_DUMP_USR
	SCmd.addCommand("EE_ADD_USR",EE_add_user);  // adds a user at a given index
	//format: EE_ADD_USR;<index>;<name>;<priv byte>;<hash>
	SCmd.addCommand("EE_READ_USR",EE_read_user);  // reads the user at <index>
	//format: EE_READ_USR;<index>
	SCmd.addCommand("EE_RMV_USR",EE_remove_user);  // removes the user at <index>
	//format: EE_RMV_USR;<index>
	SCmd.addCommand("EE_RMV_USRN",EE_remove_user_name);  // removes the user with <name>
	//format: EE_RMV_USRN;<name>
	SCmd.addCommand("EE_RMV_USRH",EE_remove_user_hash);  // removes the user with <hash>
	//format: EE_RMV_USRH;<hash>
	SCmd.addDefaultHandler(unrecognized);  // Handler for command that isn't matched  (says "What?") 

	logging(logDate());
	logging(" - ");
	loggingln("Rebooted"); 
	LCD_default();
	//populate_blank_users();
	//write_EE(0,"Evan", 15, "9a7eaa8c16bd8073d32991d7ddc8a480");
	//write_EE(1,"Charlie", 4, "dacbac186709549ff6f16b6a7df0dd1c");
	//write_EE(2,"Mike", 7, "22df7bee944f0b02bc0b9ce8b826e862");
	//write_EE(3,"Whoever", 1, "d2e8102a9b4d8bcd20d38cb1fa2b0ddc");
	//write_EE(4,"Lego", 3, "3bf21d79f6516b061a8326fda21d6af3");
	//balnk_EE_cmd();
	//command[0] = 5;
	//write_EE_cmd();
	//dump_all_EE();
	//EE_dump_users();
	//Serial.println("done");
}

void loop()
{
	SCmd.readSerial();     //process serial commands
	//Serial.println(relaysToPins[exit_relay]);
	//digitalWrite(31,1);
	if (!digitalRead(exit_button)) //exit button pressed
	{
		exit();
	}

	if (millis() - battery_start > battery_dur) //battery timer loop
	{
		check_battery;
	}

	for(int i=0;i<8;i++) //toggle timer loops
	{
		if (toggled[i])
		{
			//Serial.println(millis() - toggle_start[i]);
			if (millis() - toggle_start[i] > toggle_dur[i])
			{
				toggled[i] = 0;
				digitalWrite(relaysToPins[i], LOW);
			}
		}
	}

	if(doorbell) //doorbell timer loop
	{
  		//Serial.println(millis() - doorbell_start);
  		//Serial.println(doorbell_dur);
		if (millis() - doorbell_start > doorbell_dur)
		{
			doorbell = 0;
		}
	}
	if(doorhold) //door hold timer loop
	{
  		//Serial.println(millis() - doorhold_start);
  		//Serial.println(doorhold_dur);
		if (millis() - doorhold_start > doorhold_dur)
		{
			doorhold = 0;
		}
	}
	
	if (tag_read)
	{
		//Serial.println(millis() - RFID_start_time);
  		if (millis() - RFID_start_time > RFID_time) //RFID time out loop
		{
			//Serial.println("done");
			LCD_clear();
			LCD_displn("RFID timeout",0);
			RFID_error_out();
		}

		char keypad_button = keypad_matrix();
		if (keypad_button != ' ') //reading a button in tag read mode
		{
			if(keypad_button == '*') //escape
			{
				backspace_pin();
			}
			else if(keypad_button == '#')  //enter
			{
				if (new_tag_entry ==1)
				{
  					new_user_entry();
				}
				else
				{
					current_user_entry();
				}
			}
			else //other numbers
			{
				enter_pin(keypad_button);
			}
		}
	}
	else
	{

		char keypad_button = keypad_matrix();
		if (keypad_button != ' ')  //just reading raw buttons, not in tag reading mode
		{
  
  			if(keypad_button == '5' && exiting == 1 && can_add == 1)//button press for adding a user
			{
				enter_new_tag_entry_mode();
			}
			if(keypad_button == '2' && exiting == 1 && can_doorbell == 1)//button press for doorbell mode
			{
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
  				if(!doorhold)
  				{
					enter_doorhold_mode();
				}
  				else
  				{
    					exit_doorhold_mode();
				}
			}
			if(keypad_button == '#' && doorbell == 1)//button press in doorbell mode
			{
				exit();
				beep_reader(2,300);
			}
			if(keypad_button == '#' && doorhold == 1)//button press in door hold mode
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
		if (millis() - exit_start_time > exit_time) //exit time out loop
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
}

void temp_command()
{
	char temp[8];
	ftoa(temp,getTemp(),2);
	//dtostrf(voltage,8,2,volt); should work?
  	String printout;
	printout += "Temperature outside: ";
	printout += temp;

	logging(logDate());
	logging(" - ");
	loggingln(printout);
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


void LCD_init()
{
	analogWrite(contrast,cont_value);
	lcd.init();
	lcd.backlight(1);
	lcd.blinkOff();      // turn blinking cursor off
	lcd.clearBuffer();
	lcd.clear();
	//LCD_clear();
}

char keypad_matrix()
{
	char result = lcd.getKey(); //Read key press
	if (result != last) 
	{      //check buffer
		if (result != 0x00)  
		{  //ignore get key return of 0x00
			return result;
		}
 
		last = result;
 
	} // main test - if(a = !=b)
	else
	{
		return ' ';
	}
	//delay(10);
}

void LCD_default()
{
	//LCD_init();
	LCD_clear();
	if(doorbell)
	{
		LCD_displn("Welcome: press",0);
		LCD_displn("\"#\" to enter",1);
	}
	else if(doorhold)
	{
		LCD_displn("Door held: press",0);
		LCD_displn("\"#\" to enter",1);
	}
	else
	{
		LCD_displn("i3 Detroit",0);
		LCD_displn("Hackerspace",1);
	}
}


/*
void keypad_rst()
{
	digitalWrite(keypad_reset,0);
	delay(1);
	digitalWrite(keypad_reset,1);
}
*/
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

void check_battery()
{
	battery_start = millis();
	float voltage = (((float)analogRead(A15)*20.0)/1024.0);
	if(voltage<voltage_threshold)
	{
		//battery voltage dropped low (alarm?)
	}
	log_battery();
}

char *ftoa(char *a, double f, int precision)
{
  long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};
  
  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long desimal = abs((long)((f - heiltal) * p[precision]));
  itoa(desimal, a, 10);
  return ret;
}

void log_battery()
{
	float voltage = (((float)analogRead(A15)*20.0)/1024.0);
	char volt[8];
	ftoa(volt,voltage,2);
	//dtostrf(voltage,8,2,volt); should work?
  	String printout;
	printout += "Battery voltage: ";
	printout += volt;

	logging(logDate());
	logging(" - ");
	loggingln(printout);
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
	doorbell_start = 0;
	can_doorbell = 0;
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
	doorbell_start = millis();
	can_doorbell = 0;
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
	doorhold_start = 0;
	can_doorhold = 0;
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
	doorhold_start = millis();
	can_doorhold = 0;
}
void enter_new_tag_entry_mode()
{
	logging(logDate());
	logging(" - ");
	loggingln("new member attempt");
	LCD_clear();
	LCD_displn("New member:",0);
	LCD_displn("swipe new tag",1);
	exiting = 0;
	can_add = 0;
	digitalWrite(relaysToPins[exit_relay], LOW);
	new_tag_entry = 1;
}

void exit()
{
	logging(logDate());
	logging(" - ");
	loggingln("exit relay open");
	exiting = 1;
	exit_start_time = millis();
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
		for(int i=0;i<(sizeof(pin)/sizeof(pin[0]));i++)
		{
			toHash += pin[i];   
		}
		char hashed[32];
		toHash.toCharArray(hashed,32); 
		char *md5str = MD5::make_digest(MD5::make_hash(hashed), 16);
		//if(authenticate(md5str) == 1)
		//{
		//	exiting = 1;
		//	exit_start_time = millis();
		//	digitalWrite(relaysToPins[exit_relay], HIGH);
		//}
		//else
		//{
		//	RFID_error_out();
		//}

		for (int i=0;i<num_users;i++)
		{
			if(!user_present(i))
			{
  				String name = "New Member ";
				name += logDate();
				priv= 1;
				write_EE(i, name, priv, md5str);
				i+=num_users;
			}
		}
		//Serial.println(md5str);
		logging(logDate());
		logging(" - ");
		logging("new user ");
		logging(md5str);
		logging(" added");
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
	char hashed[32];
	toHash.toCharArray(hashed,32); 
	char *md5str = MD5::make_digest(MD5::make_hash(hashed), 16);
	if(authenticate(md5str) == 1)
	{
		logging(logDate());
		logging(" - ");
		String printname = "";
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
	for (int i = 0;i<digit;i++)
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
	pin[digit] = c - '0';
	digit++;
}

void reset_tag_reading(int reader_num)
{
	//Serial.print("tag reader ");
	//Serial.println(reader_num,HEX);
	String newtag ="";
	if(reader_num == 1)
	{
		reader1 = reader1 >> 1;
		tag = reader1;
		reader1Count = 0;
		reader1 = 0;
	}
	else if(reader_num == 2)
	{
		reader2 = reader2 >> 1;
		tag = reader2;
		reader2Count = 0;
		reader2 = 0;	
	}
	newtag+=tag;
	logging(logDate());
	logging(" - ");
	logging(newtag);
	loggingln(" tag read");
	LCD_clear();
	LCD_displn("Please Enter",0);
	LCD_displn("Pin:",1);
	tag_read = 1;
	clear_pin();
	RFID_start_time = millis();
}

void write_EE(int index, String name, byte priv, String hash)
{
	index +=1;
	char data[64] = {};
//{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	//char data[64];
	for(int i = 0;i<31;i++)
	{
		if(name[i] == 0)
		{
			i+=31;
		}
		else
		{
			data[i] = name[i];
		}
	}
	data[31] = priv;
	for(int i = 0;i<32;i++)
	{
		//if(data[i] == 0)
		//{
		//  i+=32;
		//}
		//else
		//{
		data[i+32] = hash[i];
		//}
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

void dump_all_EE()
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

void EE_dump_users()
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

void EE_read_user()
{
	char *arg; 
	arg = SCmd.next();
	serial_user_print(atoi(arg));
	//Serial.println("DONE");
}

void EE_add_user()
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



void populate_blank_users()
{
	for (int i = 0;i<num_users;i++)
		remove_user(i);
}

void EE_remove_user()
{
	char *arg; 
	arg = SCmd.next(); 
	remove_user(atoi(arg));
	Serial.println("user removed");
}

void remove_user(int i)
{
	String name = "Free";
	byte priv= 0;
	String hash = "00000000000000000000000000000000";
	write_EE(i, name, priv, hash);
}

void EE_remove_user_name()
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
				j+=((sizeof(name)/sizeof(name[0])));
			}
		}
		if(succeed)
		{
			remove_user(i);
			Serial.println("user removed");
			i+=num_users;
		}
	}
}

void EE_remove_user_hash()
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
				j+=((sizeof(hash)/sizeof(hash[0])));
			}
		}
		if(succeed)
		{
			remove_user(i);
			Serial.println("user removed");
			i+=num_users;
		}
	}
}




void serial_user_print(int index)
{
	read_EE_name(index);
	read_EE_priv(index);
	read_EE_hash(index);
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

void dump_EE(int start_address, int end_address)
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
  	index +=1;
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
	priv = EEPROM1024.read(((index+1) * entry_size) + 31);
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
		name[i] = EEPROM1024.read(i+((index+1) * entry_size));
	}
	return;
}

void read_EE_command()
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

void write_EE_cmd()
{
	for(int i = 0;i<64;i++)
	{
		EEPROM1024.write(i,command[i]);
	}
}

void balnk_EE_cmd()
{
	for(int i = 0;i<64;i++)
	{
		command[i] = 0;
	}
	for(int i = 0;i<64;i++)
	{
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
	if(priv == 1||priv == 3||priv == 5||priv == 7||priv == 9||priv == 11||priv == 13||priv == 15)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


boolean authenticate(char *md5str)
{/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////hardcode hashes

	byte hard_hash[5][33] = {{/*"e0bdc8c7b2e2d33e7889858a30aad569"*/},{},{},{},{}};/////////5 hashes
	boolean succeed = 1;
	for (int k=0;k<5;k++)
	{
		for (int j=0;j<32;j++)
		{
			if(hard_hash[k][j] != (md5str[j]))
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
  		String printname = "Failsafe";
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
					j+=32;
				}
			}
			if(succeed == 1)
			{
				read_EE_name(i);
				read_EE_priv(i);
				read_EE_hash(i);
				//Serial.print("win: ");
				//serial_user_print(i);
				if(priv == 2||priv == 3||priv == 6||priv == 7||priv == 10||priv == 11||priv == 14||priv == 15)
				{
					can_add = 1;
					//Serial.println("can add");
				}
				else
				{
					//Serial.println("cannot add");
				}
				if(priv == 4||priv == 5||priv == 6||priv == 7||priv == 12||priv == 13||priv == 14||priv == 15)
				{
					can_doorbell = 1;
					//Serial.println("can doorbell");
				}
				else
				{
					//Serial.println("cannot doorbell");
				}
				if(priv == 8||priv == 9||priv == 10||priv == 11||priv == 12||priv == 13||priv == 14||priv == 15)
				{
					can_doorhold = 1;
					//Serial.println("can door hold");
				}
				else
				{
					//Serial.println("cannot door hold");
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
	beep_repeat(2,200,3);
	LCD_default();
}


void beep_repeat(int reader,int time,int number)
{
	//Serial.print("beep repeat");
	//Serial.print(" ");
	//Serial.print(reader);
	//Serial.print(" ");
	//Serial.print(time);
	//Serial.print(" ");
	//Serial.println(number);
	switch(reader){
		case 0:{
			for(int i = 0;i<number;i++)
			{
				beep_reader(0,time);
				delay(time);
			}
		}
		case 1:{
			for(int i = 0;i<number;i++)
			{
				beep_reader(1,time);
				delay(time);
			}
		}
		case 2:{
			for(int i = 0;i<number;i++)
			{
				beep_reader(1,time);
				delay(time);
			}
		}
		default:{
		}
	}
}


void beep_reader(int reader,int d)
{
	switch(reader){
		case 0:{
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
		case 1:{
			digitalWrite(reader1led, 0);
			digitalWrite(reader1buzz, 0);
			delay(d);
			digitalWrite(reader1led, 1);
			digitalWrite(reader1buzz, 1);
		}
		case 2:{
			digitalWrite(reader2led, 0);
			digitalWrite(reader2buzz, 0);
			delay(d);
			digitalWrite(reader2led, 1);
			digitalWrite(reader2buzz, 1);
		}
		default:{
		}
	}
}


void keypad_init()
{
	//pinMode (keypad_clock, OUTPUT);
	//pinMode (keypad_input, INPUT);
	//pinMode (keypad_reset, OUTPUT);
	//digitalWrite(keypad_clock,0);
	//keypad_rst();
}

void IO_init()
{
	for (int i=0;i<sizeof(relaysToPins)/sizeof(relaysToPins[0]);i++)
	{
		pinMode(relaysToPins[i], OUTPUT);
	}
	for (int i=0;i<sizeof(analogToPins)/sizeof(analogToPins[0]);i++)
	{
		pinMode(analogToPins[i], INPUT);
	}
	pinMode(exit_button,INPUT_PULLUP);
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


void EE_read_CMD_command()
{
	read_EE_command();
	for  (int i = 0; i < 64; i++)
	{
		Serial.print(command[i], HEX);
	}
	Serial.println();
}

void clear_pin() //not needed since I use an extrenal index, I think
{
	digit = 0;
	for(int i=0;i<(sizeof(pin)/sizeof(pin[0]));i++)
	{
		pin[i] = 15;     
	}
}

void relay_on()
{
	char *arg; 
	arg = SCmd.next(); 
	if (arg != NULL && atoi(arg) < 8 && ((atoi(arg) == exit_relay && exiting == 1) ? 0 : 1)) 
	{
		digitalWrite(relaysToPins[atoi(arg)],HIGH); 
	}
}

void relay_off()
{
	char *arg; 
	arg = SCmd.next(); 
	if (arg != NULL && atoi(arg) < 8 && ((atoi(arg) == exit_relay && exiting == 1) ? 0 : 1)) 
	{
		digitalWrite(relaysToPins[atoi(arg)],LOW); 
	}
}

void beep_command()
{
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

void toggle_command()    
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
		//delay(dur);
		//digitalWrite(relaysToPins[relay], LOW);
		toggled[relay] = 1;
		toggle_dur[relay] = dur;
		toggle_start[relay] = millis();
	} 
	else 
	{
		//Serial.println("No second argument"); 
	}
}


void read_command()    
{
	int aord; 
	char *arg; 
	// Serial.println("read"); 
	arg = SCmd.next(); 
	if (arg != NULL) 
	{
		aord=atoi(arg); 
		if(!aord)
		{
			//Serial.println("analog"); 
			arg = SCmd.next(); 
			if (arg != NULL && atoi(arg) < 16) 
			{
				//Serial.print("analog pin no: A"); 
				//Serial.println(atoi(arg)); 
				Serial.println(analogRead(analogToPins[atoi(arg)]));
			}
			else 
			{
				//Serial.println("No second argument"); 
			}
		}
		if(aord)
		{
			//Serial.println("digital"); 
			arg = SCmd.next(); 
			if (arg != NULL) 
			{
				boolean valid = 1;
				for (int i=0;i<sizeof(disallowedDigitalInputs)/sizeof(disallowedDigitalInputs[0]);i++)
				{
					if(disallowedDigitalInputs[i] == atoi(arg))
						valid = 0;
				}
				if(valid && atoi(arg) < 53)
				{
					pinMode(atoi(arg), INPUT);
					//Serial.print("pin no: "); 
					//Serial.println(atoi(arg));
					Serial.println(digitalRead(atoi(arg)));
				}
				else
				{
					//Serial.println("invalid pin numver");
				}
			}
			else
			{
				//Serial.println("No second argument"); 
			}
		} 
		else 
		{
			//Serial.println("No arguments"); 
		}
	}
}


void rtc_command()    
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
			Serial.println(logDate());
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


void LCD_displn(String prints,int line)
{
	char printable[20];
	prints.toCharArray(printable,sizeof(printable));
	lcd.setCursor(line,0); // position cursor
	delay(10);
	lcd.write(printable);  // txt
	delay(10);
}

void LCD_disp(String prints)
{
	char printable[20];
	prints.toCharArray(printable,sizeof(printable));
	lcd.write(printable);  // txt
	delay(10);
}

void LCD_clear()
{
	lcd.clear(); //clear screen
	//delay(10);
}


void display_command()    
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


void print_command()    
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
		for(int i=0;i<printable.length();i++)
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
	printing(0x0d);
	printing(0x0a);
}

void logging(String tolog)
{
	Serial.print(tolog);
	printer(tolog);
}

void loggingln(String tolog)
{
	Serial.println(tolog);
	printerln(tolog);
}


void battery_command()    
{
	log_battery();
}


void printing(char buffer)
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
		case 1:{
			date+="SUN";
			break;
		}
		case 2:{
			date+="MON";
			break;
		}
		case 3:{
			date+="TUE";
			break;
		}
		case 4:{
			date+="WED";
			break;
		}
		case 5:{
			date+="THU";
			break;
		}
		case 6:{
			date+="FRI";
			break;
		}
		case 7:{
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

void callReader1Zero(){wiegand26.reader1Zero();}
void callReader1One(){wiegand26.reader1One();}
void callReader2Zero(){wiegand26.reader2Zero();}
void callReader2One(){wiegand26.reader2One();}
