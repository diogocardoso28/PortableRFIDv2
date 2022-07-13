#include <Arduino.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "SPIFFS.h"
#include "string.h"
#include "ESPAsyncWebServer.h"
#include "filesystem.h"

DynamicJsonDocument wifiConfigurations(500);
DynamicJsonDocument storedCards(5000);

extern AsyncEventSource events; // event source (Server-Sent events)

void mountFS()
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    // Lists Filesystem Content
    Serial.println("Listing Filesystem: ");
    File root = SPIFFS.open("/");

    File file = root.openNextFile();

    while (file)
    {
        Serial.print("FILE: ");
        Serial.println(file.name());

        file = root.openNextFile();
    }
    // Reads configuration
    File fileBuffer = SPIFFS.open("/settings/wifi.json", "r");
    deserializeJson(wifiConfigurations, fileBuffer);
    fileBuffer.close();

    // reads stored cards
    fileBuffer = SPIFFS.open("/storage/cards.json", "r");
    deserializeJson(storedCards, fileBuffer);
    fileBuffer.close();
}

boolean getWifiApMode()
{
    return wifiConfigurations["apMode"].as<boolean>();
}

boolean getWifiAutoApMode()
{
    return wifiConfigurations["autoAp"].as<boolean>();
}

String getApSSID()
{
    return wifiConfigurations["apSsid"].as<String>();
}

String getApPassword()
{
    return wifiConfigurations["apPassword"].as<String>();
}
String getWifiSsid() // Function that returns wifi ssid
{
    return wifiConfigurations["ssid"].as<String>();
}

String getWifiPassword() // Function that returns wifi password
{
    return wifiConfigurations["password"].as<String>();
}

DynamicJsonDocument getWifiSettings() // Returns json Document
{
    return wifiConfigurations;
}

void setWifiSsid(String ssid)
{
    wifiConfigurations["ssid"] = ssid;
}

void setWifiPassword(String password)
{
    wifiConfigurations["password"] = password;
}

void setApSsid(String ssid)
{
    wifiConfigurations["apSsid"] = ssid;
}

void setApPassword(String password)
{
    wifiConfigurations["apPassword"] = password;
}

void setAutoAp(int state)
{
    if (state)
    {
        wifiConfigurations["autoAp"] = true;
    }
    else
    {
        wifiConfigurations["autoAp"] = false;
    }
}

void setApMode(int state)
{
    if (state == 1)
    {
        wifiConfigurations["apMode"] = true;
    }
    else
    {
        wifiConfigurations["apMode"] = false;
    }
}

void saveSettings()
{

    // Open file for writing
    File file = SPIFFS.open("/settings/wifi.json", "w");
    if (!file)
    {
        Serial.println(F("Failed to create file"));
        return;
    }

    // Serialize JSON to file
    if (serializeJson(wifiConfigurations, file) == 0)
    {
        Serial.println(F("Failed to write to file"));
    }

    // Close the file
    file.close();
}

void saveCards()
{
    // Open file for writing
    File file = SPIFFS.open("/storage/cards.json", "w");
    if (!file)
    {
        Serial.println(F("Failed to create file"));
        return;
    }

    // Serialize JSON to file
    if (serializeJson(storedCards, file) == 0)
    {
        Serial.println(F("Failed to write to file"));
    }

    // Close the file
    file.close();
}

DynamicJsonDocument getStoredCards() // Returns json Document containing the stored cards object
{
    return storedCards;
}

int searchCard(DynamicJsonDocument cardUid, JsonArray cards) // returns -1 if card not found or card id if card found
{
    JsonArray uidBuffer;
    JsonArray cardUidArray;
    int uidPosition = 0;
    int cardIndex = 0;
    cardUidArray = cardUid.as<JsonArray>();

    for (JsonVariant card : cards) // loops trough existing cards
    {
        uidBuffer = card["uid"].as<JsonArray>(); // Converts current card uid to Json Array
        uidPosition = 0;
        for (JsonVariant uidByte : uidBuffer)
        {
            if (uidByte != cardUidArray[uidPosition])
                break;
            else
            {
                return cardIndex;
            }
            uidPosition++;
        }
        cardIndex++;
    }
    return -1;
};

void storeCard(byte UID[], byte uidSize, float readerFrequency, String type)
{
    DynamicJsonDocument bufferCard(400);
    DynamicJsonDocument cardUid(100);
    String buffer;

    // Converts decimal byte array to hexadecimal arduinojson array
    for (byte i = 0; i < uidSize; i++)
    {
        buffer = String(UID[i], HEX);
        buffer.toUpperCase();
        cardUid.add(buffer);
    }
    JsonArray cardsArray = storedCards["cards"].as<JsonArray>(); // Grabs all existing cards as an array

    int cardPosition = searchCard(cardUid, cardsArray);
    Serial.println(cardPosition);
    if (cardPosition == -1)
    {
        // Adds in new card
        storedCards["counter"] = storedCards["counter"].as<int>() + 1; // Increments card counter
        bufferCard["id"] = storedCards["counter"];
        bufferCard["name"] = storedCards["counter"].as<String>();
        bufferCard["uid"] = cardUid;
        bufferCard["type"] = type;
        bufferCard["frequency"] = readerFrequency;

        storedCards["cards"].add(bufferCard);

        saveCards();
        char messageBuffer[200];
        sprintf(messageBuffer, "{\"id\": %d}", storedCards["counter"].as<int>());
        events.send(messageBuffer, "newCardFound", millis());
    }
    else
    {
        char messageBuffer[200];
        sprintf(messageBuffer, "{\"id\": %d}", cardsArray[cardPosition]["id"].as<int>());
        events.send(messageBuffer, "newCardFound", millis());
    }
}

int searchCardById(int id, JsonArray cards)
{
    // Test comment :P
    int cardIndex = 0;
    for (JsonVariant card : cards) // loops trough existing cards
    {
        if (card["id"].as<int>() == id)
        {
            return cardIndex;
        }
        cardIndex++;
    }
    return -1;
}

DynamicJsonDocument getCardData(int cardId)
{
    JsonArray cards = storedCards["cards"].as<JsonArray>();
    DynamicJsonDocument cardDocument(400);

    for (JsonVariant card : cards) // loops trough existing cards
    {
        if (card["id"] == cardId)
        {
            cardDocument = card;
            return cardDocument;
        }
    }
    cardDocument["status"] = "NOT_FOUND";
    return cardDocument;
}

void updateCard(DynamicJsonDocument newCardData)
{
    JsonArray cards = storedCards["cards"].as<JsonArray>();
    int cardIndex = searchCardById(newCardData["id"].as<int>(), cards);
    Serial.println(cardIndex);
    if (cardIndex != -1)
    {
        storedCards["cards"][cardIndex]["name"] = newCardData["name"].as<String>();
        JsonArray cardUID = newCardData["uid"].as<JsonArray>();
        storedCards["cards"][cardIndex]["uid"] = cardUID;
    }

    saveCards();
}

void deleteCard(int cardID)
{
    int cardIndex = searchCardById(cardID, storedCards["cards"].as<JsonArray>());
    storedCards["cards"].remove(cardIndex);

    saveCards();
}
