//test scc 1080 displays without library
void setup() { 
 //Initialize serial and wait for port to open:
  Serial3.begin(9600); 
  pinMode(16, OUTPUT);                          // Set up the RS-485 TX enable pin on pin 16.
digitalWrite(16, HIGH);
  
  Serial3.write(0x0c); 
  Serial3.write(0x12); 
  Serial3.print("Welcome to i3Detroit"); 
  
  Serial3.write(0x10); 
  Serial3.print("   GLaDOS Online    "); 
  
  Serial3.write(0x12); 
  Serial3.print(" Begin Testing Now  "); 
  
  Serial3.print(" You May "); 
  Serial3.write(0x10);
  Serial3.print("Not");
  Serial3.write(0x12);
  Serial3.print(" Leave");
} 
void loop() { 

} 
