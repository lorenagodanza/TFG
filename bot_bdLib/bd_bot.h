#ifndef BOT_INFLUXDB_H
#define BOT_INFLUXDB_H

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <WiFiManager.h>

// Configuración de WiFi
extern const char* ssid;
extern const char* password;

// Configuración del bot de Telegram
extern const char* BOTtoken;
extern const char* CHAT_ID;
extern UniversalTelegramBot bot;

// Configuración de InfluxDB
extern const char* INFLUXDB_URL;
extern const char* INFLUXDB_TOKEN;
extern const char* INFLUXDB_ORG;
extern const char* INFLUXDB_BUCKET;

// Estructura para almacenar datos en buffer
struct DataPoint {
  String measurement;
  String value;
  bool isBool;
};

// Declaración de funciones
void setupBotAndWiFi();
bool sendToInfluxDB(const char* measurement, float value);
bool sendBoolToInfluxDB(const char* measurement, bool value);
void sendMessageToTelegram(String message);
void addToBuffer(const char* measurement, float value, bool isBool = false);
void processBuffer();
void checkWiFiAndProcessBuffer();

#endif