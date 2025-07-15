#include <DHT.h>
#include "bd_bot.h"  
#define DHTTYPE DHT11
const int dht_dpin = D1;  
DHT dht(dht_dpin, DHTTYPE);  

void setup() {
  Serial.begin(115200);
  dht.begin(); // Inicializa el sensor
  delay(1000);
  setupBotAndWiFi();

}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  Serial.print("current humidity = ");
  Serial.print(h);
  Serial.print(" current temperature = ");
  Serial.print(t);
  Serial.println("°C");
  
  sendToInfluxDB("humidity", h);
  sendToInfluxDB("temperature", t);


  // Enviar alerta si se pasan los umbrales
  if (t > 30) {
    sendMessageToTelegram("Temperatura muy alta! " + String(t) + "°C");
  } else if (t < 10) {
    sendMessageToTelegram("Temperatura muy baja! " + String(t) + "°C");
  }

  if (h > 85) {
    sendMessageToTelegram("Humedad muy alta! " + String(h) + "%");
  } else if (h < 20) {
    sendMessageToTelegram("Humedad muy baja! " + String(h) + "%");
  }
  
  delay(5000);
}