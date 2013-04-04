//RFID test code for i3Detroit RFID entry system
//only reader 2 is implemented
//outputs to serial port
//outputs to a web server at 192.168.1.177


#include <WIEGAND26.h>    // Wiegand 26 reader format libary
#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1, 177);

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);
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

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
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
    
    if (reader2Count == 4) {
      Serial.print("Read Button = ");
      Serial.println(reader2);
      button[reader2] = true;
      reader2Count = 0;
      reader2 = 0;
    }
    
      // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connnection: close");
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
                    // add a meta refresh tag, so the browser pulls again every 5 seconds:
          client.println("<meta http-equiv=\"refresh\" content=\"5\">");
          // output the value of each analog input pin
          for (int i = 0; i < 12; i++) {
            if (button[i]) {
              client.print(i);
              client.println("<br />");   
            }    
          }
              client.print("Tag ID = ");
              client.print(tag);
              client.println("<br />");
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
  }
    delay(100);
}

void callReader1Zero(){wiegand26.reader1Zero();}
void callReader1One(){wiegand26.reader1One();}
void callReader2Zero(){wiegand26.reader2Zero();}
void callReader2One(){wiegand26.reader2One();}
