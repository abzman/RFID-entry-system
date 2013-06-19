//RFID test code for i3Detroit RFID entry system
//only reader 2 is implemented
//outputs to serial port
//outputs to a web server at 192.168.1.177


#include <WIEGAND26.h>    // Wiegand 26 reader format libary



byte reader1Pins[]={19,18};          // Reader 1 connected to pins 4,5
byte reader2Pins[]={2,3};          // Reader2 connected to pins 6,7
long reader1 = 0;
int  reader1Count = 0;
long reader2 = 0;
int  reader2Count = 0;
byte reader2buzz = 6;
byte reader2led = 4;
bool access_granted = false;

//Get the interrupt number of a given digital pin on an arduino mega.
byte pinToInterrupt[21] = {-1,-1,0,1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,5,4,3,2};

bool tled = 1;
bool tbuzz = 1;
WIEGAND26 wiegand26;

void setup() {
  pinMode(reader2buzz, OUTPUT);
  pinMode(reader2led, OUTPUT);
  attachInterrupt(pinToInterrupt[reader2Pins[1]], callReader2One,  CHANGE);
  attachInterrupt(pinToInterrupt[reader2Pins[0]], callReader2Zero, CHANGE);
  wiegand26.initReaderTwo();
  //TODO: Set relays low
  Serial.begin(9600);
  Serial.println("Initializing RFID Reader Test");
 digitalWrite(reader2led, tled);
 digitalWrite(reader2buzz, tbuzz);


}

int tag = 0;
bool button[12]={};
//Keypress is 4 bits
//Tag is 26 bits
void loop() {
  
  //TODO: Make more robust by just removing digits from the buffer as they are read
  if (reader2Count >= 26) {
      Serial.print("Read Tag = ");
      Serial.println(reader2,HEX);
      tag = reader2;
      reader2Count = 0;
      reader2 = 0;
    }
    
    //esc is 10
    //enter is 11
    if (reader2Count == 4) {
      Serial.print("Read Button = ");
      Serial.println(reader2);
      button[reader2] = true;
      reader2Count = 0;
      reader2 = 0;
    }
    delay(100);
}

void callReader1Zero(){wiegand26.reader1Zero();}
void callReader1One(){wiegand26.reader1One();}
void callReader2Zero(){wiegand26.reader2Zero();}
void callReader2One(){wiegand26.reader2One();}
