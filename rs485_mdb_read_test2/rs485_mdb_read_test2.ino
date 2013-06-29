//not working (polled reads)
//test reading buttons from scc_1080 displays
//It will be rolled into scc_1080 library when done
//right now it's in "instant output full duplex mode" and mdb mode
//when it's in this mode it just outputs
//the ascii characters for 0-9, carrige return(?) for enter
//and whatever you set in firmware for up and down (I used "U" and "D" and "u" and "d") 
//this means we need a polling loop, I had polled at the beginning, and it worked...once a reboot...

#include <scc_1080.h>
int flip = 16;
SCC_1080 inside(1);
SCC_1080 outside(2);
SCC_1080 all(0);
void setup() { 
  Serial.begin(9600); 
  SCC_1080::rs485init(Serial3,9600,flip);
  SCC_1080::rs485tx(flip);
  all.vfdClear();
  all.kbEnable();
  SCC_1080::rs485tx(flip);
  inside.mdbStart();
  Serial3.write(0x1b);
  inside.mdbEnd();
  Serial3.flush();
  SCC_1080::rs485rx(flip);
} 

void loop() { 

  if(Serial3.available() > 0){
    Serial.print(Serial3.read(),HEX);
    Serial.println();
    //delay(1000);
  }
  //delay(200);
} 
