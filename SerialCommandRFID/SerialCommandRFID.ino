// <major mod from> Demo Code for SerialCommand Library
// Steven Cogswell
// May 2011
// start of test code for RFID project
// i3 Detroit
// Evan Allen

#include <scc_1080.h>
HardwareSerial *mySerial = &Serial3;
SCC_1080 inside(*mySerial,1);
SCC_1080 outside(*mySerial,2);
SCC_1080 all(*mySerial,0);

#include <SerialCommand.h>
SerialCommand SCmd;   // The demo SerialCommand object

#include <Wire.h>         // Needed for I2C Connection to the DS1307 date/time chip
#include <DS1307.h>       // DS1307 RTC Clock/Date/Time chip library
uint8_t second, minute, hour, dayOfWeek, dayOfMonth, month, year;     // Global RTC clock variables. Can be set using DS1307.getDate function.
DS1307 ds1307;        // RTC Instance

#include <WIEGAND26.h>    // Wiegand 26 reader format libary
byte reader1Pins[]={19,18};          // Reader 1 connected to pins 19,18
byte reader2Pins[]={2,3};          // Reader2 connected to pins 2,3
long reader1 = 0;
int  reader1Count = 0;
long reader2 = 0;
int  reader2Count = 0;
byte reader2buzz = 6;
byte reader2led = 4;
bool access_granted = false;
//Get the interrupt number of a given digital pin on an arduino mega.
byte pinToInterrupt[21] = {-1,-1,0,1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,5,4,3,2};
WIEGAND26 wiegand26;

int relaysToPins[8] = {31,32,33,34,35,36,37,38};
int analogToPins[16] = {54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69};
int disallowedOutputs[28] = {0,1,2,3,9,14,15,16,18,19,20,21,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69}; //serial, i2c, rs485, wiegand data pins, analog inputs
int disallowedDigitalInputs[35] = {0,1,2,3,14,15,16,18,19,20,21,31,32,33,34,35,36,37,38,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69}; //serial, i2c, rs485, relays,wiegand data pins, analog inputs

int exit_button = 9;
int exit_relay = 0;
int exit_time = 10000;

boolean exiting = 0;
unsigned long start_time = 0;

const int dataPins[] = {41,42,43,44,45,46,47,48};
const int strobePin = 39;//active low
const int busyPin = 40;//active low

void setup()
{  
	SCC_1080_init();
	Wire.begin();   // start Wire library as I2C-Bus Master
	Serial.begin(9600); 
	RFID_init();
	IO_init();
	printer_init();
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
	SCmd.addDefaultHandler(unrecognized);  // Handler for command that isn't matched  (says "What?") 

  
	all.centerLinePrint("Welcome to i3Detroit");   
	inside.centerLinePrint("Inside");
	outside.centerLinePrint("Outside");
	Serial.println("Ready"); 
}


int max_toggle = 15000; //maximum amount of time a toggle can sieze up the processor with a delay loop

int tag = 0;
boolean tag_read = 0;
int pin[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int digit = 0;


void loop()
{
	SCmd.readSerial();     //process serial commands
	if (!digitalRead(exit_button))
	{
		exiting = 1;
		start_time = millis();
		digitalWrite(relaysToPins[exit_relay], HIGH);
	}
	if (exiting)
	{
		if (millis() - start_time > exit_time)
		{
			exiting = 0;
			start_time = 0;
			digitalWrite(relaysToPins[exit_relay], LOW);
		}
	}
	if (reader2Count >= 26 && tag_read == 0) //read tag, start tag reading mode (accepting buttons as pin)
	{
		tag = reader2;
		//Serial.print("Read Tag = ");
		//Serial.println(tag,HEX);
		reader2Count = 0;
		reader2 = 0;
		tag_read = 1;
		digit = 0;
	}
	if (reader2Count >= 26 && tag_read == 1) //reset pin and tag if a tag is read mid pin-entry
	{
		tag = reader2;
		//Serial.print("Read Tag = ");
		//Serial.println(tag,HEX);
		reader2Count = 0;
		reader2 = 0;
		tag_read = 1;
		digit = 0;
	}
	if (reader2Count == 4 && tag_read == 1) //reading a button in tag read mode
	{
		if(reader2 == 10) //escape
		{
			//Serial.println("backspace");
			digit--;          
		}
		else if(reader2 == 11)  //enter
		{
			Serial.print("Tag = ");
			Serial.println(tag,HEX);
			Serial.print("Pin = ");
			for(int i=0;i<digit;i++)
			{
				Serial.print(pin[i]);      
			}
			Serial.println();
			//clear_pin();
			tag_read = 0;
			digit = 0;
		}
		else //other numbers
		{
			//Serial.print("Pin Button = ");
			//Serial.println(reader2);
			pin[digit] = reader2;
			digit++;
		}
		reader2Count = 0;
		reader2 = 0; 
	}
	if (reader2Count == 4 && tag_read == 0)  //just reading raw buttons, not in tag reading mode
	{
		Serial.print("Button = ");
		Serial.println(reader2);
		reader2Count = 0;
		reader2 = 0;
	}
	if(digit == (sizeof(pin)/sizeof(pin[0]))+1) //too big pin
	{
		//Serial.println("overflow");
		//clear_pin();
		tag_read = 0;
		digit = 0;
		int d = 300;
		beep_reader2(d);
		delay(d);
		beep_reader2(d);
		delay(d);
		beep_reader2(d);
	}
	delay(100);
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
	for(int i=0;i<(sizeof(pin)/sizeof(pin[0]));i++)
	{
		pin[0] = 0;     
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
		delay(dur);
		digitalWrite(relaysToPins[relay], LOW);
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
			logDate();
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
	Serial.println(((float)analogRead(A15)*20.0)/1024.0);
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




void logDate()
{
	ds1307.getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
	Serial.print(hour, DEC);
	Serial.print(":");
	Serial.print(minute, DEC);
	Serial.print(":");
	Serial.print(second, DEC);
	Serial.print("  ");
	Serial.print(month, DEC);
	Serial.print("/");
	Serial.print(dayOfMonth, DEC);
	Serial.print("/");
	Serial.print(year, DEC);
	Serial.print(' ');
	  
	switch(dayOfWeek){
		case 1:{
			Serial.print("SUN");
			break;
		}
		case 2:{
			Serial.print("MON");
			break;
		}
		case 3:{
			Serial.print("TUE");
			break;
		}
		case 4:{
			Serial.print("WED");
			break;
		}
		case 5:{
			Serial.print("THU");
			break;
		}
		case 6:{
			Serial.print("FRI");
			break;
		}
		case 7:{
			Serial.print("SAT");
			break;
		}  
	}
	Serial.println(" ");
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
