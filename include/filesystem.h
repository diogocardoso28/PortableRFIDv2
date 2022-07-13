#ifndef FILESYSTEM_H_INCLUDED
#define FILESYSTEM_H_INCLUDED
#include <Arduino.h>
#include <ArduinoJson.h>

void mountFS();
boolean getWifiApMode();
boolean getWifiAutoApMode();
String getApSSID();
String getApPassword();
String getWifiSsid();
String getWifiPassword();
DynamicJsonDocument getWifiSettings();
void setWifiSsid(String ssid);
void setWifiPassword(String password);
void setApSsid(String ssid);
void setApPassword(String password);
void setAutoAp(int state);
void setApMode(int state);
void saveSettings();
void saveCards();
DynamicJsonDocument getStoredCards();
int searchCard(DynamicJsonDocument cardUid, JsonArray cards);
void storeCard(byte UID[], byte uidSize, float readerFrequency, String type);
int searchCardById(int id, JsonArray cards);
DynamicJsonDocument getCardData(int cardId);
void updateCard(DynamicJsonDocument newCardData);
void deleteCard(int cardID);

#endif // FILESYSTEM_H_INCLUDED