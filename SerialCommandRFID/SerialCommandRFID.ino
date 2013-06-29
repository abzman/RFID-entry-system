// <major mod from> Demo Code for SerialCommand Library
// Steven Cogswell
// May 2011
// start of test code for RFID project
// i3 Detroit
// Evan Allen
#include <MD5.h>

#include <scc_1080.h>
HardwareSerial *mySerial = &Serial3;
SCC_1080 inside(*mySerial,1);
SCC_1080 outside(*mySerial,2);
SCC_1080 all(*mySerial,0);

#include <SerialCommand.h>
SerialCommand SCmd;   // The demo SerialCommand object

#include <Wire.h>         // Needed for I2C Connection to the DS1307 date/time chip
#include <DS1307.h>       // DS1307 RTC Clock/Date/Time chip library
#include <E24C1024.h>     // Needed for the onboard EEprom
uint8_t second, minute, hour, dayOfWeek, dayOfMonth, month, year;     // Global RTC clock variables. Can be set using DS1307.getDate function.
DS1307 ds1307;        // RTC Instance

#include <WIEGAND26.h>    // Wiegand 26 reader format libary
byte reader1Pins[]={19,18};          // Reader 1 connected to pins 19,18
byte reader2Pins[]={2,3};          // Reader2 connected to pins 2,3
long reader1 = 0;
int  reader1Count = 0;
long reader2 = 0;
int  reader2Count = 0;
const byte reader2buzz = 6;
const byte reader2led = 4;
bool access_granted = false;
//Get the interrupt number of a given digital pin on an arduino mega.
const byte pinToInterrupt[21] = {-1,-1,0,1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,5,4,3,2};
WIEGAND26 wiegand26;

const int relaysToPins[8] = {31,32,33,34,35,36,37,38};
const int analogToPins[16] = {54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69};
const int disallowedOutputs[28] = {0,1,2,3,9,14,15,16,18,19,20,21,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69}; //serial, i2c, rs485, wiegand data pins, analog inputs
const int disallowedDigitalInputs[35] = {0,1,2,3,14,15,16,18,19,20,21,31,32,33,34,35,36,37,38,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69}; //serial, i2c, rs485, relays,wiegand data pins, analog inputs

const int exit_button = 9;
const int exit_relay = 0;
const unsigned long exit_time = 5000;
boolean exiting = 0;
unsigned long exit_start_time = 0;

const unsigned long RFID_time = 60000;
unsigned long RFID_start_time = 0;

const int dataPins[] = {41,42,43,44,45,46,47,48};
const int strobePin = 39;//active low
const int busyPin = 40;//active low

int entry_size = 64;
byte hash[32];
char name[31];
byte priv;
byte command[64];

boolean doorbell = 0;
boolean can_doorbell = 0;
unsigned long doorbell_start = 0;
unsigned long doorbell_dur = (3600000*12);


unsigned long max_toggle = 15000; //maximum amount of time a toggle can sieze up the processor with a delay loop
unsigned long toggle_dur[8] = {0,0,0,0,0,0,0,0};
unsigned long toggle_start[8] = {0,0,0,0,0,0,0,0};
boolean toggled[8] = {0,0,0,0,0,0,0,0};


boolean doorhold = 0;
boolean can_doorhold = 0;
unsigned long doorhold_start = 0;
unsigned long doorhold_dur = (60000*10);

long voltage_threshold = 12.5;
unsigned long battery_start = 0;
unsigned long battery_dur = (3600000);

long tag = 0;
boolean tag_read = 0;
byte pin[8] = {15,15,15,15,15,15,15,15}; //untypable character, unlike 0
byte digit = 0;
boolean new_tag_entry = 0;
boolean can_add = 0;
int num_users = 500;


void setup()
{  
	SCC_1080_init();
	Wire.begin();   // start Wire library as I2C-Bus Master
	Serial.begin(9600); 
	RFID_init();
	IO_init();
	printer_init();
	battery_start = millis();
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
	//format: DISP;<0=all, 1=inside, 2=outside>;<0=left, 1=center, 2=right>;<string (or no argument to clear the display)>
	SCmd.addCommand("PRINT",print_command);  // uses printer
	//format: PRINT;<string>
	SCmd.addCommand("BAT",battery_command);  // reads battery voltage
	//format: BAT
	SCmd.addCommand("BEEP",beep_command);  // beeps reader2 for <duration> in delay(arg) format
	//format: BEEP
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

  
	all.centerLinePrint("Welcome to i3Detroit");   
	inside.centerLinePrint("Inside");
	outside.centerLinePrint("Outside");
	Serial.println("Ready"); 
	//populate_blank_users();
	//write_EE(0,"Evan", 15, "2dfcd8b17808e9bd50e47f2c94879111");
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
	if (millis() - battery_start > battery_dur)
	{
		check_battery;
	}
  
  
	SCmd.readSerial();     //process serial commands
	if (!digitalRead(exit_button)) //start exiting
	{
		exit();
	}
	for(int i=0;i<8;i++)
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
	if(doorbell == 1)
	{
  		//Serial.println(millis() - doorbell_start);
  		//Serial.println(doorbell_dur);
		if (millis() - doorbell_start > doorbell_dur)
		{
			doorbell = 0;
		}
	}
	if(doorhold == 1)
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
  		if (millis() - RFID_start_time > RFID_time) //RFID timeout
		{
			//Serial.println("done");
			RFID_error_out();
		}

		if (reader2Count == 4) //reading a button in tag read mode
		{
			if(reader2 == 10) //escape
			{
				backspace_pin();
			}
			else if(reader2 == 11)  //enter
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
				enter_pin();
			}
			reader2Count = 0;
			reader2 = 0; 
		}
	}
	else
	{
		if (reader2Count == 4)  //just reading raw buttons, not in tag reading mode
		{
  
  			if(reader2 == 5 && exiting == 1 && can_add == 1)//button press for adding a user
			{
				enter_new_tag_entry_mode();
			}
			if(reader2 == 2 && exiting == 1 && can_doorbell == 1)//button press for doorbell mode
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
			if(reader2 == 3 && exiting == 1 && can_doorhold == 1)//button press for door hold mode
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
			if(reader2 == 11 && doorbell == 1)//button press in doorbell mode
			{
				exit();
			}
			if(reader2 == 11 && doorhold == 1)//button press in door hold mode
			{
				exit();
			}
			Serial.print("Button = ");
			Serial.println(reader2);
			reader2Count = 0;
			reader2 = 0;
		}
	}

	if(digit == (sizeof(pin)/sizeof(pin[0]))+1) //too big pin
	{
		Serial.println("overflow");
		RFID_error_out();
	}
	if (exiting)
	{
		if (millis() - exit_start_time > exit_time) //stop exiting (shut door relay off)
		{
			Serial.println("exit timed out");
			exiting = 0;
			can_add = 0;
			can_doorhold = 0;
			can_doorbell = 0;
			//exit_start_time = 0; //not needed
			digitalWrite(relaysToPins[exit_relay], LOW);
		}
	}
	if (reader2Count >= 26) //read tag for new input, start tag reading mode (accepting buttons as pin)
	{
		Serial.println("tag read");
		reset_tag_reading();
	}
	delay(100);
}


void check_battery()
{
	battery_start = millis();
	float voltage = (((float)analogRead(A15)*20.0)/1024.0);
	if(voltage<voltage_threshold)
	{
		//battery voltage dropped low
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
	//dtostrf(voltage,8,2,volt);
  	String printout;
	printout += "Battery voltage: ";
	printout += volt;
	printout += " Time: ";
	printout += logDate();
	Serial.println(printout);
	//parallel printout
	//display output
}
void exit_doorbell_mode()
{
	Serial.println("exit doorbell mode");
	doorbell = 0;
	doorbell_start = 0;
	can_doorbell = 0;
}
void enter_doorbell_mode()
{
	Serial.println("doorbell mode");
	doorbell =1;
	doorbell_start = millis();
	can_doorbell = 0;
}

void exit_doorhold_mode()
{
	Serial.println("exit door hold mode");
	doorhold = 0;
	doorhold_start = 0;
	can_doorhold = 0;
}
void enter_doorhold_mode()
{
	Serial.println("door hold mode");
	doorhold =1;
	doorhold_start = millis();
	can_doorhold = 0;
}
void enter_new_tag_entry_mode()
{
	Serial.println("new tag");
	exiting = 0;
	can_add = 0;
	digitalWrite(relaysToPins[exit_relay], LOW);
	new_tag_entry = 1;
}

void exit()
{
	Serial.println("exiting");
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
		beep_reader2(d);
		delay(d);
		beep_reader2(d);
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
  				String name = "New User ";
				name += logDate();
				priv= 1;
				write_EE(i, name, priv, md5str);
				i+=num_users;
			}
		}
		//Serial.println(md5str);
		Serial.println("added!");
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
		exit();
	}
	else
	{
		RFID_error_out();
	}
	Serial.println(md5str);
	//Serial.println();
	//Serial.println(test);
	for(int i=0;i<(sizeof(hashed)/sizeof(hashed[0]));i++)
	{
		hashed[i] = 0;   
	}
	toHash = "";
	RFID_reset();
}



void backspace_pin()
{
	pin[digit] = 15;
	//Serial.println("backspace");
	if(digit >0)
	{
		digit--;
		pin[digit] = 15;
	}
	//for(int i=0;i<(sizeof(pin)/sizeof(pin[0]));i++)
	//{
	//	Serial.print(pin[i]);      
	//}
}

void enter_pin()
{
	Serial.print("Pin Button = ");
	Serial.println(reader2);
	pin[digit] = reader2;
	digit++;
}

void reset_tag_reading()
{
	tag = reader2;
	Serial.print("Read Tag = ");
	Serial.println(tag,HEX);
	reader2Count = 0;
	reader2 = 0;
	tag_read = 1;
	clear_pin();
	RFID_start_time = millis();
}

void write_EE(int index, String name, byte priv, String hash)
{
	index +=1;
	char data[64] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
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
  
	Serial.println("Writing data");
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
	Serial.print((char)(priv+48));
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
		//Serial.print(" :");
		//Serial.println(data);
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
{//////////////////////////////////////////////////////////////////////////////////////////////////hardcode hashes
	for (int i=0;i<num_users;i++)
	{
		if(user_present(i))
		{
			read_EE_hash(i);
			boolean succeed = 1;
			for (int j=0;j<32;j++)
			{
				if(hash[j] != (md5str[j]))
				{
					Serial.print("fail: ");
					Serial.println(i,DEC);
					//Serial.println(j,DEC);
					//Serial.println(hash[j],HEX);
					//Serial.println((md5str[j]),HEX);
					succeed = 0;
					j+=32;
				}
			}
			if(succeed == 1)
			{
				Serial.print("win: ");
				serial_user_print(i);
				if(priv == 2||priv == 3||priv == 6||priv == 7||priv == 10||priv == 11||priv == 14||priv == 15)
				{
					can_add = 1;
					Serial.println("can add");
				}
				if(priv == 4||priv == 5||priv == 6||priv == 7||priv == 12||priv == 13||priv == 14||priv == 15)
				{
					can_doorbell = 1;
					Serial.println("can doorbell");
				}
				if(priv == 8||priv == 9||priv == 10||priv == 11||priv == 12||priv == 13||priv == 14||priv == 15)
				{
					can_doorhold = 1;
					Serial.println("can door hold");
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
}

void RFID_error_out()
{
	RFID_reset();
	int d = 300;
	beep_reader2(d);
	delay(d);
	beep_reader2(d);
	delay(d);
	beep_reader2(d);
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
	digitalWrite(reader2led, 1); //active low
	digitalWrite(reader2buzz, 1); //active low
	attachInterrupt(pinToInterrupt[reader2Pins[1]], callReader2One,  CHANGE);
	attachInterrupt(pinToInterrupt[reader2Pins[0]], callReader2Zero, CHANGE);
	wiegand26.initReaderTwo();
}

void SCC_1080_init()
{
	SCC_1080::rs485init(*mySerial,9600,16);
	SCC_1080::rs485tx(16);
	all.kbDisable();
	all.vfdClear();
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

void beep_reader2(int d)
{
	digitalWrite(reader2led, 0);
	digitalWrite(reader2buzz, 0);
	delay(d);
	digitalWrite(reader2led, 1);
	digitalWrite(reader2buzz, 1);
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
		beep_reader2(atoi(arg));
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
		if (atoi(arg) < max_toggle)
		{
			dur=atoi(arg); 
		}
		else
		{
			dur=1000;
		}
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


void display_command()    
{
	char *arg; 
	//Serial.println("display"); 
	arg = SCmd.next();
	if (arg != NULL) 
	{
		if (atoi(arg) == 0)
		{
			//Serial.println("all");
			arg = SCmd.next();
			if (arg != NULL) 
			{
				if (atoi(arg) == 0) 
				{
					arg = SCmd.next();
					if (arg != NULL) 
					{
						all.leftLinePrint(arg); 
					}
				}
				else if (atoi(arg) == 1) 
				{
					arg = SCmd.next();
					if (arg != NULL) 
					{
						all.centerLinePrint(arg);
					} 
				}
				else if (atoi(arg) == 2) 
				{
					arg = SCmd.next();
					if (arg != NULL) 
					{
						all.rightLinePrint(arg); 
					}
				}
				else
				{
					//Serial.println("invalid entry");
				}
			} 
			else
			{
				all.vfdClear();
			}
		}
		else if (atoi(arg) == 1)
		{
			//Serial.println("inside");
			arg = SCmd.next();
			if (arg != NULL) 
			{
				if (atoi(arg) == 0) 
				{
					arg = SCmd.next();
					if (arg != NULL) 
					{
						inside.leftLinePrint(arg); 
					}
				}
				else if (atoi(arg) == 1) 
				{
					arg = SCmd.next();
					if (arg != NULL) 
					{
						inside.centerLinePrint(arg); 
					}
				}
				else if (atoi(arg) == 2) 
				{
					arg = SCmd.next();
					if (arg != NULL) 
					{
						inside.rightLinePrint(arg); 
					}
				}
				else
				{
					//Serial.println("invalid entry");
				}
			} 
			else
			{
				inside.vfdClear();
			}
		}
		else if (atoi(arg) == 2)
		{
			//Serial.println("outside");
			arg = SCmd.next();
			if (arg != NULL) 
			{
				if (atoi(arg) == 0) 
				{
					arg = SCmd.next();
					if (arg != NULL) 
					{
						outside.leftLinePrint(arg); 
					}
				}
				else if (atoi(arg) == 1) 
				{
					arg = SCmd.next();
					if (arg != NULL) 
					{
						outside.centerLinePrint(arg); 
					}
				}
				else if (atoi(arg) == 2) 
				{
					arg = SCmd.next();
					if (arg != NULL) 
					{
						outside.rightLinePrint(arg); 
					}
				}
				else
				{
					//Serial.println("invalid entry");
				}
			} 
			else
			{
				outside.vfdClear();
			}
		}
	}  
	else 
	{
		//Serial.println("No arguments"); 
	}
}


void print_command()    
{
	char *arg; 
	//Serial.println("print"); 
	arg = SCmd.next(); 
	if (arg != NULL) 
	{
		String printable = arg;
		for(int i=0;i<(sizeof(printable)/sizeof(printable[0]));i++)
		{
			printer(printable[i]);
		}
		printer(0x0d);
		printer(0x0a);
	} 
	else {
		//Serial.println("No arguments"); 
	}
}

void battery_command()    
{
	log_battery();
}


void printer(char buffer)
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
	date+=hour;
	date+=":";
	date+=minute;
	date+=":";
	date+=second;
	date+="  ";
	date+=month;
	date+="/";
	date+=dayOfMonth;
	date+="/";
	date+=year;
	date+=' ';
	  
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
	date+=" ";
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
