
//const int rst = 2;
//const int clk = 3;
const int wrt = 4;
//const int ena = 5;
const int dataPins[] = {6,7,8,9,10,11,12,13};
//const int fla = 22;
//const int addrPins[] = {23,24,25,26};
const int addrPins[] = {23,24,25};

char buffer = 0x00;
int addr[] = {0,0,0,1};
int data[] = {0,0,0,0,0,0,0,0};

// the setup routine runs once when you press reset:
void setup() {          
  Serial.begin(9600);  
  // initialize the digital pin as an output.
  //pinMode(rst, OUTPUT);   
  //pinMode(clk, OUTPUT);
  pinMode(wrt, OUTPUT);
  //pinMode(ena, OUTPUT);
  //pinMode(fla, OUTPUT);
  for (int i = 0; i < sizeof(dataPins) / sizeof(dataPins[0]); ++i)
    pinMode(dataPins[i], OUTPUT);
  for (int i = 0; i < sizeof(addrPins) / sizeof(addrPins[0]); ++i)
    pinMode(addrPins[i], OUTPUT);    
    
  //digitalWrite(rst, HIGH);   
  //digitalWrite(clk, LOW);
  digitalWrite(wrt, HIGH);
  //digitalWrite(ena, LOW);
  //digitalWrite(fla, HIGH);
  for (int i = 0; i < sizeof(dataPins) / sizeof(dataPins[0]); ++i)
    digitalWrite(dataPins[i], LOW);
  for (int i = 0; i < sizeof(addrPins) / sizeof(addrPins[0]); ++i)
    digitalWrite(addrPins[i], LOW);  
    
  //void display_rst();
}

// the loop routine runs over and over again forever:
void loop() {
  
  data[0] = 0;
  data[1] = 0;
  data[2] = 0;
  data[3] = 1;
  data[4] = 0;
  data[5] = 0;
  data[6] = 0;
  data[7] = 1;
  for (int i = 0; i < 8; ++i)
    digitalWrite(dataPins[i], data[i]);
  //buffer = 0x00;
    
  addr[0] = 0;
  addr[1] = 0;
  addr[2] = 0;
  //addr[3] = 1;
  for (int i = 0; i < 3; ++i)
    digitalWrite(addrPins[i], addr[i]);
  display_wrt();
  
}


/*void display_rst(){
  digitalWrite(rst, LOW); 
  delay(1);  
  digitalWrite(rst, HIGH);   
  return;
}*/

void display_wrt(){
  digitalWrite(wrt, LOW); 
  delay(1);  
  digitalWrite(wrt, HIGH);   
  return;
}
