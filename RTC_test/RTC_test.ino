//RTC test program used for i3 Detroit RFID entry system

#include <Wire.h>         // Needed for I2C Connection to the DS1307 date/time chip
#include <DS1307.h>       // DS1307 RTC Clock/Date/Time chip library

uint8_t second, minute, hour, dayOfWeek, dayOfMonth, month, year;     // Global RTC clock variables. Can be set using DS1307.getDate function.

DS1307 ds1307;        // RTC Instance

// the setup routine runs once when you press reset:
void setup() {                
  Wire.begin();   // start Wire library as I2C-Bus Master

  //ds1307.setDateDs1307(0,34,20,7,16,3,13);         
  /*  Sets the date/time (needed once at commissioning)
   
   uint8_t second,        // 0-59
   uint8_t minute,        // 0-59
   uint8_t hour,          // 1-23
   uint8_t dayOfWeek,     // 1-7
   uint8_t dayOfMonth,    // 1-28/29/30/31
   uint8_t month,         // 1-12
   uint8_t year);         // 0-99
   */
   
  Serial.begin(9600);  // Set up the USB Serial output at 8,N,1,57600bps

}

// the loop routine runs over and over again forever:
void loop() {
  logDate();
  Serial.println();
  delay(1000);
}

void logDate()
{
  ds1307.getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  Serial.print(hour, DEC);
  Serial.print(":");
  Serial.print(minute, DEC);
  Serial.print(":");
  Serial.print(second, DEC);
  Serial.print("  ");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(dayOfMonth, DEC);
  Serial.print("/");
  Serial.print(year, DEC);
  Serial.print(' ');
  
  switch(dayOfWeek){

    case 1:{
     Serial.print("SUN");
     break;
           }
    case 2:{
     Serial.print("MON");
     break;
           }
    case 3:{
     Serial.print("TUE");
     break;
          }
    case 4:{
     Serial.print("WED");
     break;
           }
    case 5:{
     Serial.print("THU");
     break;
           }
    case 6:{
     Serial.print("FRI");
     break;
           }
    case 7:{
     Serial.print("SAT");
     break;
           }  
  }
  
  Serial.print(" ");

}
