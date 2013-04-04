//test of early prototype for scc_1080 libraries
void setup() { 
 //Initialize serial and wait for port to open:
  Serial3.begin(9600);                          // RS-485 serial port setup
  pinMode(16, OUTPUT);                          // Set up the RS-485 TX enable pin on pin 16.
  digitalWrite(16, HIGH);                       // RS-485 set to transmit
    
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
  goto2();
  bargraph(56);
  mdbEnd();
  
} 
void loop() { 

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

void clear() {
  //clears display and puts cursor at position 1
  Serial3.write(0x0c); //end address
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

void goto1() {
  //puts cursor at beginning of first line
  Serial3.write(0x14); //set cursor position
  Serial3.write(0x30);
  Serial3.write(0x31);
}

void goto2() {
  //puts cursor at beginning of second line
  Serial3.write(0x14); //set cursor position
  Serial3.write(0x32);
  Serial3.write(0x31);
}

void goto3() {
  //puts cursor at beginning of third line
  Serial3.write(0x14); //set cursor position
  Serial3.write(0x34);
  Serial3.write(0x31);
}

void goto4() {
  //puts cursor at beginning of fourth line
  Serial3.write(0x14); //set cursor position
  Serial3.write(0x36);
  Serial3.write(0x31);
}

void bargraph(int input) {
  //takes int from 0 to 99, graphs on given line
  String bar = "";
  bar.concat(input);
  Serial3.write(0x1b);
  Serial3.write(0x35);
  Serial3.print(bar);
}
