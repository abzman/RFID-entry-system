//test of early prototype for scc_1080 libraries
void setup() {
  
  rs485init();
  rs485tx();

  mdbStart(2);
  kbDisable();
  clear();
  centerLinePrint("Welcome to i3Detroit"); 
  
  blink();
  centerLinePrint("GLaDOS Online"); 
  knilb();
  
  centerLinePrint("Begin Testing Now"); 
  
  Serial3.print("  You May "); 
  
  blink();
  Serial3.print("Not");
  knilb();
  
  Serial3.print(" Leave ");
  mdbEnd();
  
  mdbStart(1);
  kbDisable();
  clear();
  centerLinePrint("Radiation Level");
  mdbEnd();
  
} 
void loop() { 
mdbStart(1);
  gotoLine(2);
  for(int i=0;i<100;i++) {
    bargraph(i);
    delay(1000);
  }
  mdbEnd();
} 

void clear() {
  //clears display and puts cursor at position 1
  Serial3.write(0x0c); //end address
}

void blink() {
  //starts text blinking
  Serial3.write(0x10); 
}

void knilb() {
  //stops text blinking
  Serial3.write(0x12); 
}

void mdbStart(byte address) {
  Serial3.write(0x02); //start address
  Serial3.write(address); //address
}

void mdbEnd() {
  Serial3.write(0x04); //end address
}

void centerLinePrint(String input) {
  //centers text on one line (odd gets shifted further right)
  if(input.length() % 2) {
    input = " " + input;
  }
  
  if(input.length() == 20) {
    Serial3.print(input);
  }
  
  if(input.length() < 20) {
    for(int i = 0;i = ((20 - input.length())/2);i++) {
      input = " " + input + " ";
    }
    Serial3.print(input);
  }
  
}

void leftLinePrint(String input) {
  //left justifies text on one line
  if(input.length() == 20) {
    Serial3.print(input);
  }
  
  if(input.length() < 20) {
    for(int i = 0;i = ((20 - input.length()));i++) {
      input = input + " ";
    }
    Serial3.print(input);
  }
  
}

void rightLinePrint(String input) {
  //right justifies text on one line
  if(input.length() == 20) {
    Serial3.print(input);
  }
  
  if(input.length() < 20) {
    for(int i = 0;i = ((20 - input.length()));i++) {
      input = " " + input;
    }
    Serial3.print(input);
  }
  
}

void kbEnable() {
  //enables keypad
  Serial3.write(0x1b); //escape code
  Serial3.write(0x32);
}

void kbDisable() {
  //disables keypad
  Serial3.write(0x1b); //escape code
  Serial3.write(0x31);
}

void gotoLine(int line) {
  //Line is valid from 1 to 4
  //puts cursor at beginning of nth line
  Serial3.write(0x14); //set cursor position
  Serial3.write(0x30 + line*2 -2);
  Serial3.write(0x31);
}

void bargraph(int input) {
  //takes int from 0 to 99, graphs on given line
  String bar = "";
  if (input < 10) {
    bar.concat("0");
  }
  bar.concat(input);
  Serial3.write(0x1b);
  Serial3.write(0x35);
  Serial3.print(bar);
}

void cursorOn() {
  //turns on cursor
  Serial3.write(0x15);
}

void cursorOff() {
  //turns off cursor
  Serial3.write(0x16);
}

void rs485init() {
  //initializes the sn65hvd05 on the openaccess control shield at given baud rate
  Serial3.begin(9600);                          // RS-485 serial port setup
  pinMode(16, OUTPUT);                          // Set up the RS-485 TX enable pin on pin 16.
}

void rs485tx() {
  //sets the sn65hvd05 on the openaccess control shield to transmit
  digitalWrite(16, HIGH);                       // RS-485 set to transmit
}

void rs485rx() {
  //sets the sn65hvd05 on the openaccess control shield to recieve
  digitalWrite(16, LOW);                       // RS-485 set to recieve
}
