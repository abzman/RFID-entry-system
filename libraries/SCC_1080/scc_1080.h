#ifndef	_SCC_1080_H_ 
#define	_SCC_1080_H_ 

#include <Arduino.h>

class SCC_1080 {
	public:
		SCC_1080();
		SCC_1080(HardwareSerial &port);
		SCC_1080(HardwareSerial &port, int id);
		SCC_1080(int id);
		~SCC_1080();

		void vfdClear();
		void blink();
		void knilb();
		void centerLinePrint(String input);
		void leftLinePrint(String input);
		void rightLinePrint(String input);
		void kbEnable();
		void kbDisable();
		void escReset();
		void gotoLine(int line);
		void bargraph(int input);
		void cursorOn();
		void cursorOff();
		void mdbStart();
		void mdbEnd();

		static void rs485init(HardwareSerial &port, long rate, int pin);
		static void rs485tx(int pin);
		static void rs485rx(int pin);
	private:
		HardwareSerial *_port;
		int _id;

};


#endif

