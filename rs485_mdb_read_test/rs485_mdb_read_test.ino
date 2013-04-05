//partially working
//test reading buttons from scc_1080 displays
//It will be rolled into scc_1080 library when done
//right now it's in "instant output full duplex mode" and mdb mode
//when it's in this mode it just outputs
//the ascii characters for 0-9, carrige return(?) for enter
//and whatever you set in firmware for up and down (I used "U" and "D" and "u" and "d") 
//this means we need a polling loop, I had polled at the beginning, and it worked...once a reboot...

#include <scc_1080.h>

SCC_1080 inside(1);
SCC_1080 outside(2);
SCC_1080 all(0);
void setup() { 
  Serial.begin(9600); 
  SCC_1080::rs485init(Serial3,9600,16);
  SCC_1080::rs485tx(16);
  all.vfdClear();
  all.kbEnable();
  
  SCC_1080::rs485rx(16);
} 


void loop() { 
  if(Serial3.available() > 0){
    Serial.write(Serial3.read());
    Serial.println();
  }
} 
