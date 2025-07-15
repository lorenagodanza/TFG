#include "bd_bot.h"
#include <vector>

// Configuración del bot de Telegram
const char* BOTtoken = "7773832019:AAFph3-_Tux99WRPbZmyIfHAdsdD92AMmfw";
const char* CHAT_ID = "572950870";

// Configuración de InfluxDB
const char* INFLUXDB_URL = "http://MacBook-Pro-de-Lorena.local:8086";
const char* INFLUXDB_TOKEN = "mVvggDDebtBEq55cVThEUVzqhFqvk7YnD5GnBe_m2MkCfjyGafOZuSdBQrEtI7ZBFwf8c5Iuc_CJ3WBReSYNAw==";
const char* INFLUXDB_ORG = "udc";
const char* INFLUXDB_BUCKET = "my_bucket";

// Buffer para almacenar datos cuando no hay conexión
std::vector<DataPoint> dataBuffer;
const int MAX_BUFFER_SIZE = 50; // Máximo de puntos de datos a almacenar

WiFiClient client;
WiFiClientSecure client2;
UniversalTelegramBot bot(BOTtoken, client2);
WiFiManager wifiManager;

void setupBotAndWiFi() {
  client2.setInsecure();
  wifiManager.autoConnect("ESP8266-Access-Point");
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("IP address: " + WiFi.localIP().toString());
  
  // Iniciar el bot de Telegram
  bot.sendMessage(CHAT_ID, "Bot preparado", "");
  
  // Procesar buffer si hay datos pendientes
  if(!dataBuffer.empty()) {
    processBuffer();
  }
}

void addToBuffer(const char* measurement, float value, bool isBool) {
  if(dataBuffer.size() >= MAX_BUFFER_SIZE) {
    dataBuffer.erase(dataBuffer.begin());
    Serial.println("Buffer lleno - Dato más antiguo eliminado");
  }
  
  DataPoint newData;
  newData.measurement = measurement;
  newData.value = String(value);
  newData.isBool = isBool;
  dataBuffer.push_back(newData);
  
  Serial.println("Dato añadido al buffer. Tamaño del buffer: " + String(dataBuffer.size()));
}

//verificamos si la base de datos está disponible
bool isInfluxDBAvailable() {
  if(WiFi.status() != WL_CONNECTED) {
    return false;
  }
  
  String url = String(INFLUXDB_URL) + "/ping";
  HTTPClient http;
  http.begin(client, url);
  int httpCode = http.GET();
  http.end();
  
  return (httpCode == 204); // InfluxDB responde 204 a /ping cuando está disponible
}

void processBuffer() {
  if(!isInfluxDBAvailable()) {
    Serial.println("InfluxDB no disponible, no se puede procesar el buffer");
    return;
  }
  
  Serial.println("Procesando buffer... Tamaño: " + String(dataBuffer.size()));
  
  while(!dataBuffer.empty() && isInfluxDBAvailable()) {
    DataPoint data = dataBuffer.front();
    bool success;
    
    if(data.isBool) {
      success = sendBoolToInfluxDB(data.measurement.c_str(), data.value == "true");
    } else {
      success = sendToInfluxDB(data.measurement.c_str(), data.value.toFloat());
    }
    
    if(success) {
      dataBuffer.erase(dataBuffer.begin());
      Serial.println("Dato enviado correctamente. Buffer restante: " + String(dataBuffer.size()));
    } else {
      Serial.println("Error al enviar dato, reintentando más tarde");
      break;
    }
    
    delay(100); 
  }
}

void checkWiFiAndProcessBuffer() {
  static unsigned long lastCheck = 0;
  const unsigned long CHECK_INTERVAL = 30000; // 30 segundos
  
  if(millis() - lastCheck >= CHECK_INTERVAL || !dataBuffer.empty()) {
    lastCheck = millis();
    
    if(WiFi.status() != WL_CONNECTED) {
      Serial.println("Reconectando WiFi...");
      WiFi.reconnect();
      delay(5000); // Esperar un poco para la reconexión
    }
    
    if(isInfluxDBAvailable() && !dataBuffer.empty()) {
      processBuffer();
    }
  }
}

void sendMessageToTelegram(String message) {
  if(WiFi.status() == WL_CONNECTED) {
    bot.sendMessage(CHAT_ID, message, "");
  } else {
    Serial.println("No se puede enviar mensaje a Telegram - WiFi desconectado");
  }
}

bool sendToInfluxDB(const char* measurement, float value) {
  if(!isInfluxDBAvailable()) {
    addToBuffer(measurement, value);
    return false;
  }
  
  char data[128];
  snprintf(data, sizeof(data), "%s value=%f", measurement, value);
  
  String url = String(INFLUXDB_URL) + "/api/v2/write?org=" + INFLUXDB_ORG + 
               "&bucket=" + INFLUXDB_BUCKET + "&precision=ns";

  HTTPClient http;
  http.begin(client, url);
  http.addHeader("Authorization", "Token " + String(INFLUXDB_TOKEN));
  http.addHeader("Content-Type", "text/plain");

  int httpCode = http.POST(data);
  bool success = (httpCode == 204);
  
  if(!success) {
    Serial.print("[InfluxDB] Error en el envío. Código: ");
    Serial.println(httpCode);
    Serial.println("[InfluxDB] Error: " + http.errorToString(httpCode));
    addToBuffer(measurement, value);
  }

  http.end();
  return success;
}

bool sendBoolToInfluxDB(const char* measurement, bool value) {
  if(!isInfluxDBAvailable()) {
    addToBuffer(measurement, value ? 1.0 : 0.0, true);
    return false;
  }
  
  char data[128];
  snprintf(data, sizeof(data), "%s value=%s", measurement, value ? "true" : "false");

  String url = String(INFLUXDB_URL) + "/api/v2/write?org=" + INFLUXDB_ORG + 
               "&bucket=" + INFLUXDB_BUCKET + "&precision=ns";

  HTTPClient http;
  http.begin(client, url);
  http.addHeader("Authorization", "Token " + String(INFLUXDB_TOKEN));
  http.addHeader("Content-Type", "text/plain");

  int httpCode = http.POST(data);
  bool success = (httpCode == 204);

  if(!success) {
    Serial.print("[InfluxDB] Error en el envío. Código: ");
    Serial.println(httpCode);
    Serial.println("[InfluxDB] Error: " + http.errorToString(httpCode));
    addToBuffer(measurement, value ? 1.0 : 0.0, true);
  }

  http.end();
  return success;
}