#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "rfid.h"
#include "filesystem.h"

extern byte *uidToWrite;
extern int readerStatus;

MFRC522 mfrc522(PIN_SS, PIN_RST); // Create MFRC522 instance

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void printHex(byte *buffer, byte bufferSize)
{
    for (byte i = 0; i < bufferSize; i++)
    {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte *buffer, byte bufferSize)
{
    for (byte i = 0; i < bufferSize; i++)
    {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], DEC);
    }
}

void initializeModule()
{
    SPI.begin();                       // Init SPI bus
    mfrc522.PCD_Init();                // Init MFRC522
    delay(4);                          // Optional delay. Some board do need more time after init to be ready, see Readme
    mfrc522.PCD_DumpVersionToSerial(); // Show details of PCD - MFRC522 Card Reader details
    Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}

void listenForCard()
{
    float readerFrequency = 13.56;
    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
    if (!mfrc522.PICC_IsNewCardPresent())
    {
        return;
    }

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial())
    {
        return;
    }

    Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    String cardType = mfrc522.PICC_GetTypeName(piccType);
    Serial.println(cardType);

    // Check is the PICC of Classic MIFARE type
    if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
        piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
        piccType != MFRC522::PICC_TYPE_MIFARE_4K)
    {
        Serial.println(F("Your tag is not of type MIFARE Classic."));
        return;
    }
    Serial.print(F("In hex: "));
    printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();

    storeCard(mfrc522.uid.uidByte, mfrc522.uid.size, readerFrequency, cardType);
    // Dump debug info about the card; PICC_HaltA() is automatically called

    // mfrc522.PICC_DumpToSerial(&(mfrc522.uid));

    // Halt PICC
    mfrc522.PICC_HaltA();

    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
}

bool writeCard()
{

    Serial.println("Wating for card");
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
    {
        return false;
    }
    if (mfrc522.MIFARE_SetUid(uidToWrite, (byte)4, true))
    {
        Serial.println(F("Wrote new UID to card."));
    }

    uidToWrite = (byte *)malloc(0);
    readerStatus = 1;
    return true;
}