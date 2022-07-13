#ifndef WEBSERVER_H_
#define WEBSERVER_H_
#include <Arduino.h>
#include <ArduinoJson.h>
void initializeWifi();
void initializeWifiAp();
void setWriteUID(int cardId);
void startWebServer();
void updateStatus();
void cardReadData(int cardId);
void cardWrittenSuccess();
void stopWrite();
#endif // WEBSERVER_H_