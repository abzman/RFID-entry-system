/*
  EEPROM1024.pde
  AT24C1024 EEPROM Benchmark Sketch 
  
  Our project page is here:
  http://www.arduino.cc/playground/Code/I2CEEPROM24C1024
  
  From the datasheet:

  The AT24C1024B provides 1,048,576 bits of serial electrically 
  erasable and programmable read only memory (EEPROM) organized 
  as 131,072 words of 8 bits each. The device’s cascadable 
  feature allows up to four devices to share a common two-wire 
  bus.
  
  http://www.atmel.com/dyn/resources/prod_documents/doc5194.pdf
  
*/
#include <Arduino.h>
#include <Wire.h>
#include <E24C1024.h>

unsigned long address = 0;
int entry_size = 64;

void setup()
{
  // Make sure we aren't reading old data
  Serial.begin(9600);
  Serial.println();
  Serial.println("E24C1024 Library Benchmark Sketch");
  Serial.println();
  write_EE(0,"Evan", 9, "2dfcd8b17808e9bd50e47f2c94879111");
  write_EE(1,"Charlie", 4, "dacbac186709549ff6f16b6a7df0dd1c");
  write_EE(2,"Mike", 3, "22df7bee944f0b02bc0b9ce8b826e862");
  write_EE(3,"Whoever", 1, "d2e8102a9b4d8bcd20d38cb1fa2b0ddc");
  write_EE(4,"Lego", 0, "3bf21d79f6516b061a8326fda21d6af3");
for(int i = 0;i<100;i++)
{
  read_EE_hash(i);
  read_EE_priv(i);
  read_EE_name(i);
}
}

void loop()
{
}

void write_EE(int index, char name[], byte priv, char hash[])
{
  char data[64] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  //char data[64];
  for(int i = 0;i<31;i++)
  {
    if(name[i] == 0)
    {
      i+=31;
    }
    else
    {
      data[i] = name[i];
    }
  }
  data[31] = priv;
  for(int i = 0;i<32;i++)
  {
    //if(data[i] == 0)
    //{
    //  i+=32;
    //}
    //else
    //{
      data[i+32] = hash[i];
    //}
  }
  
  int start_address = index * entry_size;
  int end_address = start_address + entry_size;
  
  //Serial.print("Writing data:");
  for (int address = start_address; address < end_address; address++)
  {
    EEPROM1024.write(address, data[address % entry_size]);
    delay(3);
  }
  //Serial.println();
}

void dump_all_EE()
{
  for (address = 0; address < 131072; address++)
  {
    char data;
    data = EEPROM1024.read(address);
      Serial.print(address);
      Serial.print(" :");
      Serial.print(data, HEX);
      //Serial.print(" :");
      //Serial.println(data);
  }
  //Serial.println("DONE");
}

void dump_EE(int start_address, int end_address)
{
  for (address = start_address; address < end_address; address++)
  {
    char data;
    data = EEPROM1024.read(address);
      Serial.print(address);
      Serial.print(" :");
      Serial.print(data, HEX);
      //Serial.print(" :");
      //Serial.println(data);
  }
  //Serial.println("DONE");
}

void read_EE_hash(int index)
{
  int start_address = index * entry_size;
  int end_address = start_address + entry_size;
  
  for ((address = start_address + 32); address < end_address; address++)
  {
    char data;
    data = EEPROM1024.read(address);
      Serial.print(data);
  }
  Serial.println();
}
void read_EE_priv(int index)
{
  int start_address = index * entry_size;
  int end_address = start_address + entry_size;
  
    char data;
    data = EEPROM1024.read(start_address + 31);
      Serial.println(data, HEX);
}

void read_EE_name(int index)
{
int start_address = index * entry_size;
  int end_address = start_address + entry_size;
  
  for (address = start_address; address < (start_address + 31); address++)
  {
    char data;
    data = EEPROM1024.read(address);
      Serial.print(data);
  }
  Serial.println();
}
