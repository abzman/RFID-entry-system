//not working
//test reading buttons from scc_1080 displays, will be rolled into scc_1080 library when done

char buffer[15];
void setup() { 
 //Initialize serial and wait for port to open:
  Serial3.begin(9600); 
  pinMode(16, OUTPUT); // Set up the RS-485 TX enable pin on pin 16.
  
  Serial.begin(9600);
  digitalWrite(16, HIGH);
  Serial3.write(0x0c);
  Serial3.write(0x1b);
  Serial3.write(0x32);
} 


void loop() { 
  delay(2);
  digitalWrite(16, HIGH); //transmit
  
  Serial3.write(0x0c); //clear screen
  Serial3.write("HI"); //test

  
  Serial3.write(0x0b); //read characters
  digitalWrite(16, LOW);
      Serial.write("HI");
  if(Serial3.available() > 0){
    Serial.print(Serial3.readBytes(buffer,3),HEX);
    Serial.println();
  }
} 
