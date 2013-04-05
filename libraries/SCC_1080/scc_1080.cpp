#include <scc_1080.h>

SCC_1080::SCC_1080() {
  //Default init on Serial3 and not in mdb mode
  _port = &Serial3;
  _id = -1;
}

SCC_1080::SCC_1080(HardwareSerial &port) {
  //Init on &Port and not in mdb mode
  _port = &port;
  _id = -1;
}

SCC_1080::SCC_1080(int id) {
  //Init on Serial3 in mdb mode in id <id>
  _port = &Serial3;
  _id = id;
}

SCC_1080::SCC_1080(HardwareSerial &port, int id) {
  //Init on &port in mdb mode in id <id>
  _port = &port;
  _id = id;
}

SCC_1080::~SCC_1080() {
}

void SCC_1080::vfdClear() {
  mdbStart();
  //clears display and puts cursor at position 1
  (*_port).write(0x0c); //end address
  mdbEnd();
}

void SCC_1080::blink() {
  mdbStart();
  //starts text blinking
  (*_port).write(0x10);
  mdbEnd();
}

void SCC_1080::knilb() {
  mdbStart();
  //stops text blinking
  (*_port).write(0x12);
  mdbEnd();
}

void SCC_1080::mdbStart() {
	if (_id != -1) {
		(*_port).write(0x02); //start address
		(*_port).write(_id); //address
	}
}

void SCC_1080::mdbEnd() {
	if (_id != -1)
 		(*_port).write(0x04); //end address
		(*_port).flush();	//for reading
}

void SCC_1080::centerLinePrint(String input) {
  mdbStart();
  //centers text on one line (odd gets shifted further right)

  if(input.length() > 20) {
    (*_port).println(input);
  }

  if(input.length() == 20) {
    (*_port).print(input);
  }

  if(input.length() % 2) {
    input = " " + input;
  }

  if(input.length() < 20) {
    for(int i = 0;i = ((20 - input.length())/2);i++) {
      input = " " + input + " ";
      }
    (*_port).print(input);
  }

  mdbEnd();
}

void SCC_1080::leftLinePrint(String input) {
  mdbStart();
  //left justifies text on one line
  if(input.length() == 20) {
    (*_port).print(input);
  }
  
  if(input.length() < 20) {
    for(int i = 0;i = ((20 - input.length()));i++) {
      input = input + " ";
    }
    (*_port).print(input);
  }
  mdbEnd();
}

void SCC_1080::rightLinePrint(String input) {
  mdbStart();
  //right justifies text on one line
  if(input.length() == 20) {
    (*_port).print(input);
  }
  
  if(input.length() < 20) {
    for(int i = 0;i = ((20 - input.length()));i++) {
      input = " " + input;
    }
    (*_port).print(input);
  }
  mdbEnd();
}

void SCC_1080::kbEnable() {
  mdbStart();
  //enables keypad
  (*_port).write(0x1b); //escape code
  (*_port).write(0x32);
  mdbEnd();
}

void SCC_1080::escReset() {
  mdbStart();
  //resets display
  (*_port).write(0x1b); //escape code
  (*_port).write(0x30);
  mdbEnd();
}

void SCC_1080::kbDisable() {
  mdbStart();
  //disables keypad
  (*_port).write(0x1b); //escape code
  (*_port).write(0x31);
  mdbEnd();
}

void SCC_1080::gotoLine(int line) {
  mdbStart();
  //Line is valid from 1 to 4
  //puts cursor at beginning of nth line
  (*_port).write(0x14); //set cursor position
  (*_port).write(0x30 + line*2 -2);
  (*_port).write(0x31);
  mdbEnd();
}

void SCC_1080::bargraph(int input) {
  mdbStart();
  //takes int from 0 to 99, graphs on given line
  String bar = "";
  if (input < 10) {
    bar.concat("0");
  }
  bar.concat(input);
  (*_port).write(0x1b);
  (*_port).write(0x35);
  (*_port).print(bar);
  mdbEnd();
}

void SCC_1080::cursorOn() {
  mdbStart();
  //turns on cursor
  (*_port).write(0x15);
  mdbEnd();
}

void SCC_1080::cursorOff() {
  mdbStart();
  //turns off cursor
  (*_port).write(0x16);
  mdbEnd();
}

void SCC_1080::rs485init(HardwareSerial &port, long rate, int pin) {
  //initializes the sn65hvd05 on the openaccess control shield at given baud rate
  port.begin(rate);                          // RS-485 serial port setup
  pinMode(pin, OUTPUT);                          // Set up the RS-485 TX enable pin on pin 16.
}

void SCC_1080::rs485tx(int pin) {
  //sets the sn65hvd05 on the openaccess control shield to transmit
  digitalWrite(pin, HIGH);                       // RS-485 set to transmit
}

void SCC_1080::rs485rx(int pin) {
  //sets the sn65hvd05 on the openaccess control shield to recieve
  digitalWrite(pin, LOW);                       // RS-485 set to recieve
}