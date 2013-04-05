//partially working
//test reading buttons from scc_1080 displays
//It will be rolled into scc_1080 library when done
//right now it's in "instant output full duplex mode"
//when it's in this mode it just outputs
//the ascii characters for 0-9, carrige return(?) for enter
//and whatever you set in firmware for up and down (I used "U" and "D") 
//this means we need a polling loop, I had polled 

#include <scc_1080.h>

SCC_1080 input();
char buffer[15];
void setup() { 
  Serial.begin(9600); 
  SCC_1080::rs485init(Serial3,9600,16);
  SCC_1080::rs485tx(16);
  input.vfdClear();
  input.kbEnable();
  SCC_1080::rs485rx(16);
  if(Serial3.available() > 0){
    Serial3.read();//gets rid of the first zero, no idea why it's there
  }
} 


void loop() { 
  delay(1000);//to prove it buffers
  if(Serial3.available() > 0){
    Serial.write(Serial3.read());
    Serial.println();
  }
} 
