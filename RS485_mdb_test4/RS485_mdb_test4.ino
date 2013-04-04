//example of using scc_1080 and a telnet connection as a remote display
//telnet to 192.168.1.177 port 23
//use <esc><enter> to exit

#include <SPI.h>
#include <Ethernet.h>
#include <scc_1080.h>

HardwareSerial *mySerial = &Serial3;
SCC_1080 inside(*mySerial,2);
SCC_1080 outside(*mySerial,1);
SCC_1080 all(*mySerial,0);


byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1, 177);
IPAddress gateway(192,168,1, 1);
IPAddress subnet(255, 255, 0, 0);
// telnet defaults to port 23
EthernetServer server(23);
boolean alreadyConnected = false; // whether or not the client was connected previously

void setup() {

  SCC_1080::rs485init(*mySerial,9600,16);
  SCC_1080::rs485tx(16);
  Serial.begin(9600);
   while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  // initialize the ethernet device
  Ethernet.begin(mac, ip, gateway, subnet);
  // start listening for clients
  server.begin();
  
  inside.kbDisable();
  inside.vfdClear();
  inside.centerLinePrint("Initializing..."); 
  
  outside.kbDisable();
  outside.vfdClear();
  outside.centerLinePrint("Initializing...");
}
int cur = 0;
char buf[255] = {};
void loop() {
  
    // wait for a new client:
  EthernetClient client = server.available();

  // when the client sends the first byte, say hello:
  if (client) {
    if (!alreadyConnected) {
      // clear out the input buffer:
      client.flush();   
      all.vfdClear(); 
      outside.centerLinePrint("We have a new client");
      inside.leftLinePrint("Hello, client!"); 
      alreadyConnected = true;
    } 

    if (client.available() > 0) {
      // read the bytes incoming from the client:
      char tmp = client.read();
      if (tmp == 0x1B) {
        Serial.println("Exiting...");
        inside.leftLinePrint("Exiting...");
        alreadyConnected = false;
        client.stop();
      } else if ((tmp >= 0x20 && tmp <= 0x7E)) {
        buf[cur] = tmp;
        ++cur;
      }
      if (tmp == '\n' || cur == 254) {
        Serial.println(buf);
        inside.leftLinePrint(buf); 
        int i = 0;
        for (;i<255;++i) buf[i] = 0;
        cur = 0;
      }
    }
  }
}
