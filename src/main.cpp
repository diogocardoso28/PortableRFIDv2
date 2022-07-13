#include <Arduino.h>
#include "filesystem.h"
#include "rfid.h"
#include "webserver.h"

int readerStatus = 1;
int apStatus = 0;

int statusMillis = 0;

byte *uidToWrite;
void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600); // Initialize serial communications with the PC
  // *uidToWrite = NULL;
  initializeModule(); // Initializes SPI and rfid module
  mountFS();          // Initializes LITTLE FS file system
  initializeWifi();
}

void loop()
{

  if (statusMillis + 5000 <= millis())
  {
    statusMillis = millis();
    updateStatus();
  }
  if (readerStatus == 1) // Checks if it shoud listen for cards (Based on UI Input)
  {
    listenForCard(); // Listens for new rfid Card
  }
  else if (readerStatus == 2)
  {
    if (writeCard())
    {
      updateStatus();
      cardWrittenSuccess();
    }
  }
}