#include <Arduino.h>
#include <Update.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "ESPAsyncWebServer.h"
#include "webserver.h"
#include "filesystem.h"
#include "FS.h"
#include "SPIFFS.h"

AsyncWebServer server(80);
AsyncEventSource events("/events"); // event source (Server-Sent events)

extern int apStatus;
extern int readerStatus;

extern byte *uidToWrite;

void initializeWifi()
{
    Serial.print("Apmode: ");
    Serial.println(getWifiApMode());
    if (getWifiApMode() == false)
    {
        Serial.println("Attempting to Connect to Wifi!");

        // Attenpts wifi connection
        WiFi.begin(getWifiSsid().c_str(), getWifiPassword().c_str());

        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.write(".");
            if (millis() > 20000)
            {
                WiFi.disconnect();
                Serial.println(" ");
                if (getWifiAutoApMode())
                {
                    Serial.println("Error while connecting to wifi, initializing Wifi Hotspot");
                    initializeWifiAp();
                    startWebServer();
                    return;
                }
                else
                {
                    Serial.println("Error while connecting to wifi!");
                    return;
                }
            }
        }

        Serial.println("Wifi Connected Successfully!");
        Serial.print("Ip address: ");
        Serial.println(WiFi.localIP());
        Serial.println(WiFi.RSSI());
        Serial.println("");
        startWebServer();
    }
    else
    {
        Serial.println("Starting AP mode!");
        initializeWifiAp();
        startWebServer();
    }
}

void initializeWifiAp()
{
    apStatus = 1; // Instanciates that AP was enabled

    IPAddress local_IP(1, 2, 3, 4);
    IPAddress gateway(1, 1, 1, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_IP, gateway, subnet);

    WiFi.softAP(getApSSID().c_str(), getApPassword().c_str());
    Serial.println(WiFi.softAPIP());
}

// Sets uid to write to new cards
void setWriteUID(int cardId)
{
    byte *backupByte = uidToWrite;
    int uidSize;
    DynamicJsonDocument card = getCardData(cardId);
    JsonArray uid = card["uid"];
    uidSize = uid.size();
    uidToWrite = (byte *)malloc(uidSize);
    if (uidToWrite == NULL)
    {
        uidToWrite = backupByte;
    }

    char buffer[5];
    for (int i = 0; i < uidSize; i++)
    {
        uid[i].as<String>().toCharArray(buffer, 5);
        uidToWrite[i] = strtol(buffer, 0, 16);
    }

    readerStatus = 2;
    updateStatus();

    free(backupByte);
}

void startWebServer()
{
    Serial.println("Starting WebServer!");

    server.serveStatic("/", SPIFFS, "/webInterface/").setDefaultFile("index.html");

    // Gets wifi prefferences
    server.on("/get_wifi_settings", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  AsyncResponseStream *response = request->beginResponseStream("application/json");
                  serializeJson(getWifiSettings(), *response); //Serializes the document returned by getWifiSettings and stores it to response to be sent back
                  request->send(response); });

    // Gets stored Cards
    server.on("/get_stored_cards", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  AsyncResponseStream *response = request->beginResponseStream("application/json");
                  serializeJson(getStoredCards(), *response); //Serializes the document returned by getWifiSettings and stores it to response to be sent back
                  request->send(response); });

    // Get specific card Cards
    server.on("/get_card_by_id", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  int id;
                  if (request->hasParam("cardId"))
                  {
                      id = request->getParam("cardId")->value().toInt();
                      AsyncResponseStream *response = request->beginResponseStream("application/json");
                      serializeJson(getCardData(id), *response); //Serializes the document returned by getWifiSettings and stores it to response to be sent back
                      request->send(response);
                  }
                  else
                  {
                      request->send(400); //responds with bad request code
                  } });

    // Starts Event feature
    events.onConnect([](AsyncEventSourceClient *client)
                     {
                         if (client->lastId())
                         {
                             Serial.printf("Client reconnected! Last message ID that it gat is: %u\n", client->lastId());
                         }
                         // and set reconnect delay to 1 second
                         client->send("success", NULL, millis(), 1000);
                         updateStatus(); });
    server.addHandler(&events);

    server.on("/save_settings", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                  // Gets ssid
                  if (request->hasParam("wifiSSID", true))
                  {
                      setWifiSsid(request->getParam("wifiSSID", true)->value());
                  }

                  // gets password

                  if (request->hasParam("wifiPasword", true))
                  {
                      setWifiPassword(request->getParam("wifiPasword", true)->value());
                  }
                  // gets ap ssid
                  if (request->hasParam("apSsid", true))
                  {
                      setApSsid(request->getParam("apSsid", true)->value());
                  }

                  // gets ap password
                  if (request->hasParam("apPasword", true))
                  {
                      setApPassword(request->getParam("apPasword", true)->value());
                  }

                  // guets automatic ap parameter
                  if (request->hasParam("autoAp", true))
                  {
                      setAutoAp(request->getParam("autoAp", true)->value().toInt());
                  }

                  // guets ap mode parameter
                  if (request->hasParam("apMode", true))
                  {
                      setApMode(request->getParam("apMode", true)->value().toInt());
                  }

                  //Saves settings
                  saveSettings();
                  AsyncResponseStream *response = request->beginResponseStream("application/json");

                  DynamicJsonDocument buffer(50);
                  buffer["success"] = true;
                  serializeJson(buffer, *response); //Serializes the document containing the success status

                  request->send(response); });

    // save card Changes
    server.on("/save_card", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                  //   checks if request contains card object if not thorws bad response
                  if (request->hasParam("cardObject", true))
                  {
                      //   setWifiSsid(request->getParam("wifiSSID", true)->value());
                      DynamicJsonDocument cardObject(400);
                      deserializeJson(cardObject, request->getParam("cardObject", true)->value());
                      updateCard(cardObject);
                      request->send(200); // responds with 200 OK
                  }
                  else
                  {
                      request->send(400); // responds with bad request code
                  } });

    // Get specific card Cards
    server.on("/delete_card_by_id", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  int id;
                  if (request->hasParam("cardId"))
                  {
                      id = request->getParam("cardId")->value().toInt();
                      deleteCard(id);
                      request->send(200);
                  }
                  else
                  {
                      request->send(400); //responds with bad request code
                  } });

    server.on("/write_card_by_id", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  int id;
                  if (request->hasParam("cardId"))
                  {
                      id = request->getParam("cardId")->value().toInt();
                      //   deleteCard(id);
                      setWriteUID(id);

                      request->send(200);
                  }
                  else
                  {
                      request->send(400); //responds with bad request code
                  } });

    server.on("/write_stop", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  stopWrite();
                  updateStatus();
                  request->send(200); });

    server.begin();
}

void updateStatus()
{
    // Serial.println("Sending Status Update!"); // Debug Message
    char messageBuffer[200];
    char readerStatusString[50];
    if (readerStatus == 1)
    {
        sprintf(readerStatusString, "ready");
    }
    else if (readerStatus == 2)
    {
        sprintf(readerStatusString, "write");
    }
    else
    {
        sprintf(readerStatusString, "off");
    }
    sprintf(messageBuffer, "{\"readerStatus\": \"%s\", \"rssi\": %d, \"batteryStatus\": 100, \"apMode\": %d }", readerStatusString, WiFi.RSSI(), apStatus);
    events.send(messageBuffer, "statusUpdate", millis());
}

void cardReadData(int cardId)
{
    char messageBuffer[200];
    sprintf(messageBuffer, "{\"id\": %d}", cardId);
    events.send(messageBuffer, "newCardFound", millis());
}

void cardWrittenSuccess()
{
    events.send("", "cardWrittenSuccess", millis());
}

void stopWrite()
{
    readerStatus = 1;
    uidToWrite = (byte *)malloc(0);
}