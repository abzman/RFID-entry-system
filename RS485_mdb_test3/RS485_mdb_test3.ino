//example use of newly written scc_1080 header

#include <scc_1080.h>

HardwareSerial *mySerial = &Serial3;
SCC_1080 inside(*mySerial,1);
SCC_1080 outside(*mySerial,2);
SCC_1080 all(*mySerial,0);


void setup() {
  
  SCC_1080::rs485init(*mySerial,9600,16);
  SCC_1080::rs485tx(16);

  inside.kbDisable();
  inside.vfdClear();
  inside.centerLinePrint("Welcome to i3Detroit"); 
  
  inside.blink();
  inside.centerLinePrint("GLaDOS Online"); 
  inside.knilb();
  
  inside.centerLinePrint("Begin Testing Now"); 
  
  Serial3.print("  You May "); 
  
  inside.blink();
  Serial3.print("Not");
  inside.knilb();
  
  Serial3.print(" Leave ");
  
  outside.kbDisable();
  outside.vfdClear();
  outside.centerLinePrint("Radiation Level");
}

void loop() {
  outside.gotoLine(2);
  for(int i=0;i<100;i++) {
    outside.bargraph(i);
    delay(1000);
  }
  all.vfdClear();
  all.gotoLine(2);
  all.centerLinePrint("ALL YOUR BASE");
  all.centerLinePrint("ARE BELONG TO US");
  delay(10000);
}
