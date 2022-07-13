#ifndef RFID_H_INCLUDED
#define RFID_H_INCLUDED
#include <Arduino.h>

#define PIN_SS 5
#define PIN_RST 0

// function Declaration
void printHex(byte *buffer, byte bufferSize);
void printDec(byte *buffer, byte bufferSize);
void initializeModule();
void listenForCard();
bool writeCard();

#endif // !RFID_H_INCLUDED