//Printer Pins
const int dataPins[] = {22,23,24,25,26,27,28,29};
const int strobePin = 39;//active low
//const int lineFeedPin = 40;//active low
//const int errorPin = 41;
//const int resetPin = 42;
//const int ackPin = 43;
const int busyPin = 44;//active low
//const int paperOutPin = 45;
//const int selPin = 46;//high if printer is on

//End Printer Pins

char buffer = 0x00;

void setup() {
  //Printer I/O Setup
  for (int i = 0; i < sizeof(dataPins) / sizeof(dataPins[0]); ++i)
    pinMode(dataPins[i], OUTPUT);
  pinMode(strobePin, OUTPUT);
  pinMode(busyPin, INPUT);     
  //pinMode(ackPin, INPUT);
  
  digitalWrite(strobePin, HIGH);
 //End Printer I/O Setup
 
 //Setup init, lf, sel
 //init is ??
 //lf is high
 //sel is low
 
 //Use Serial input to stream to the printer for testing
 Serial.begin(9600);
}

void loop() {
  if (Serial.available() > 0) {
    buffer = Serial.read();
    
    //Write out our buffer
    for (int i = 0; i < sizeof(dataPins) / sizeof(dataPins[0]); ++i)
      digitalWrite(dataPins[i], (buffer & (1 << i)));
    buffer = 0x00;
    
    //Block and wait for the printer to be ready
    while (digitalRead(busyPin));
    
    //Tell the printer to write out the data
    digitalWrite(strobePin, LOW);
    delay(10);
    digitalWrite(strobePin, HIGH);
    
    //Wait for an acknolidge from the printer
    //while (digitalRead(ackPin));
    //delay(5);
    
    //Reset the data lines
    for (int i = 0; i < sizeof(dataPins) / sizeof(dataPins[0]); ++i) 
      digitalWrite(dataPins[i], LOW);
  }
}
