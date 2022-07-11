#include "ArduinoJson.h"
#include "LittleFS.h"
#include "Ticker.h"
#include "ZHNetwork.h"
#include "ZHSmartHomeProtocol.h"

//***********************КОНФИГУРАЦИЯ***********************//
// Раскомментируйте только один:
//#define WHITE
#define RGB
//#define RGBW
//#define RGBWW

// Укажите соответствующие GPIO:
#if defined(WHITE) || defined(RGBW) || defined(RGBWW)
#define COLD_WHITE_PIN 4
#endif
#if defined(RGBWW)
#define WARM_WHITE_PIN 13
#endif
#if defined(RGB) || defined(RGBW) || defined(RGBWW)
#define RED_PIN 12
#define GREEN_PIN 14
#define BLUE_PIN 15
#endif

const char *myNetName{"SMART"}; // Укажите имя сети ESP-NOW.
//***********************************************************//

void onBroadcastReceiving(const char *data, const byte *sender);
void onUnicastReceiving(const char *data, const byte *sender);
void loadStatus(void);
void saveStatus(void);
void restart(void);
void attributesMessage(void);
void keepAliveMessage(void);
void statusMessage(void); //
String getValue(String data, char separator, int index);
void changeLedState(void);

String firmware{"1.0"};
bool ledStatus{false};
byte brightness{255};
uint16_t temperature{255};
byte red{255};
byte green{255};
byte blue{255};
byte gatewayMAC[6]{0};

ZHNetwork myNet;
Ticker attributesMessageTimer;
Ticker keepAliveMessageTimer;
Ticker statusMessageTimer; //
Ticker restartTimer;

void setup()
{
  analogWriteRange(255);
#if defined(WHITE) || defined(RGBW) || defined(RGBWW)
  pinMode(COLD_WHITE_PIN, OUTPUT);
#endif
#if defined(RGBWW)
  pinMode(WARM_WHITE_PIN, OUTPUT);
#endif
#if defined(RGB) || defined(RGBW) || defined(RGBWW)
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
#endif
  LittleFS.begin();
  loadStatus();
  changeLedState();
  myNet.begin(myNetName);
  myNet.setOnBroadcastReceivingCallback(onBroadcastReceiving);
  myNet.setOnUnicastReceivingCallback(onUnicastReceiving);
}

void loop()
{
  myNet.maintenance();
}

void onBroadcastReceiving(const char *data, const byte *sender)
{
  PayloadsData incomingData;
  os_memcpy(&incomingData, data, sizeof(PayloadsData));
  if (incomingData.deviceType != GATEWAY || myNet.macToString(gatewayMAC) == myNet.macToString(sender))
    return;
  if (incomingData.payloadsType == KEEP_ALIVE)
  {
    os_memcpy(gatewayMAC, sender, 6);
    attributesMessage();
    keepAliveMessage();
    statusMessage();
    attributesMessageTimer.attach(3600, attributesMessage);
    keepAliveMessageTimer.attach(60, keepAliveMessage);
    statusMessageTimer.attach(300, statusMessage);
  }
}

void onUnicastReceiving(const char *data, const byte *sender)
{
  PayloadsData incomingData;
  os_memcpy(&incomingData, data, sizeof(PayloadsData));
  if (incomingData.deviceType != GATEWAY || myNet.macToString(gatewayMAC) != myNet.macToString(sender))
    return;
  StaticJsonDocument<sizeof(incomingData.message)> json;
  if (incomingData.payloadsType == SET)
  {
    deserializeJson(json, incomingData.message);
    if (json["set"])
      ledStatus = json["set"] == "ON" ? true : false;
    if (json["brightness"])
      brightness = json["brightness"];
    if (json["temperature"])
      temperature = json["temperature"];
    if (json["rgb"])
    {
      red = getValue(String(json["rgb"].as<String>()).substring(0, sizeof(incomingData.message)).c_str(), ',', 0).toInt();
      green = getValue(String(json["rgb"].as<String>()).substring(0, sizeof(incomingData.message)).c_str(), ',', 1).toInt();
      blue = getValue(String(json["rgb"].as<String>()).substring(0, sizeof(incomingData.message)).c_str(), ',', 2).toInt();
    }
    changeLedState();
    statusMessage();
  }
  if (incomingData.payloadsType == UPDATE)
  {
#if defined(WHITE) || defined(RGBW) || defined(RGBWW)
    digitalWrite(COLD_WHITE_PIN, LOW);
#endif
#if defined(RGBWW)
    digitalWrite(WARM_WHITE_PIN, LOW);
#endif
#if defined(RGB) || defined(RGBW) || defined(RGBWW)
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(BLUE_PIN, LOW);
#endif
    myNet.update();
    attributesMessageTimer.detach();
    keepAliveMessageTimer.detach();
    statusMessageTimer.detach();
    restartTimer.once(300, restart);
  }
  if (incomingData.payloadsType == RESTART)
    restart();
}

void loadStatus()
{
  if (!LittleFS.exists("/status.json"))
  {
    saveStatus();
    return;
  }
  File file = LittleFS.open("/status.json", "r");
  String jsonFile = file.readString();
  StaticJsonDocument<256> json;
  deserializeJson(json, jsonFile);
  ledStatus = json["status"];
  brightness = json["brightness"];
  temperature = json["temperature"];
  red = json["red"];
  green = json["green"];
  blue = json["blue"];
  file.close();
}

void saveStatus()
{
  StaticJsonDocument<256> json;
  json["status"] = ledStatus;
  json["brightness"] = brightness;
  json["temperature"] = temperature;
  json["red"] = red;
  json["green"] = green;
  json["blue"] = blue;
  File file = LittleFS.open("/status.json", "w");
  serializeJsonPretty(json, file);
  file.close();
}

void restart()
{
  ESP.restart();
}

void attributesMessage()
{
  PayloadsData outgoingData{LED, ATTRIBUTES};
  StaticJsonDocument<sizeof(outgoingData.message)> json;
#if defined(TYPE8266)
  json["MCU"] = "ESP8266";
#endif
#if defined(TYPE8285)
  json["MCU"] = "ESP8285";
#endif
  json["MAC"] = myNet.getNodeMac();
  json["Firmware"] = firmware;
  json["Library"] = myNet.getFirmwareVersion();
  char buffer[sizeof(outgoingData.message)];
  serializeJsonPretty(json, buffer);
  os_memcpy(outgoingData.message, buffer, sizeof(outgoingData.message));
  char temp[sizeof(PayloadsData)];
  os_memcpy(temp, &outgoingData, sizeof(PayloadsData));
  myNet.sendUnicastMessage(temp, gatewayMAC);
}

void keepAliveMessage()
{
  PayloadsData outgoingData{LED, KEEP_ALIVE};
  char temp[sizeof(PayloadsData)];
  os_memcpy(temp, &outgoingData, sizeof(PayloadsData));
  myNet.sendUnicastMessage(temp, gatewayMAC);
}

void statusMessage()
{
  PayloadsData outgoingData{LED, STATE};
  StaticJsonDocument<sizeof(outgoingData.message)> json;
  json["state"] = ledStatus ? "ON" : "OFF";
  json["brightness"] = brightness;
  json["temperature"] = temperature;
  json["rgb"] = String(red) + "," + String(green) + "," + String(blue);
  char buffer[sizeof(outgoingData.message)];
  serializeJsonPretty(json, buffer);
  os_memcpy(outgoingData.message, buffer, sizeof(outgoingData.message));
  char temp[sizeof(PayloadsData)];
  os_memcpy(temp, &outgoingData, sizeof(PayloadsData));
  myNet.sendUnicastMessage(temp, gatewayMAC);
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void changeLedState(void)
{
  if (ledStatus)
  {
    if (red == 255 && green == 255 && blue == 255)
    {
#if defined(WHITE) || defined(RGBW)
      analogWrite(COLD_WHITE_PIN, brightness);
#endif
#if defined(RGBWW)
      analogWrite(COLD_WHITE_PIN, map(brightness, 0, 255, 0, map(temperature, 500, 153, 0, 255)));
      analogWrite(WARM_WHITE_PIN, map(brightness, 0, 255, 0, map(temperature, 153, 500, 0, 255)));
#endif
#if defined(RGB)
      analogWrite(RED_PIN, map(red, 0, 255, 0, brightness));
      analogWrite(GREEN_PIN, map(green, 0, 255, 0, brightness));
      analogWrite(BLUE_PIN, map(blue, 0, 255, 0, brightness));
#endif
#if defined(RGBW) || defined(RGBWW)
      digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(BLUE_PIN, LOW);
#endif
    }
    else
    {
#if defined(WHITE)
      analogWrite(COLD_WHITE_PIN, brightness);
#endif
#if defined(RGBW) || defined(RGBWW)
      digitalWrite(COLD_WHITE_PIN, LOW);
#if defined(RGBWW)
      digitalWrite(WARM_WHITE_PIN, LOW);
#endif
#endif
#if defined(RGB) || defined(RGBW) || defined(RGBWW)
      analogWrite(RED_PIN, map(red, 0, 255, 0, brightness));
      analogWrite(GREEN_PIN, map(green, 0, 255, 0, brightness));
      analogWrite(BLUE_PIN, map(blue, 0, 255, 0, brightness));
#endif
    }
  }
  else
  {
#if defined(WHITE) || defined(RGBW) || defined(RGBWW)
    digitalWrite(COLD_WHITE_PIN, LOW);
#endif
#if defined(RGBWW)
    digitalWrite(WARM_WHITE_PIN, LOW);
#endif
#if defined(RGB) || defined(RGBW) || defined(RGBWW)
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(BLUE_PIN, LOW);
#endif
  }
  saveStatus();
}