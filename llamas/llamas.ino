#include "bd_bot.h"

int Led = LED_BUILTIN;
int sensorD0 = D1;  // Salida digital del sensor
int sensorA0 = A0;  // Salida analógica del sensor
int valDigital;
int valAnalogico;
int umbralLlama = 150; // Ajusta según la sensibilidad deseada

void setup() {
  Serial.begin(115200);
  setupBotAndWiFi();
  pinMode(Led, OUTPUT);
  pinMode(sensorD0, INPUT);
}

void loop() {
  valDigital = digitalRead(sensorD0);
  valAnalogico = analogRead(sensorA0);

  //necesario para que non detecte falsos positivos con luz ambiente alta
  bool llama=(valDigital == HIGH && valAnalogico<umbralLlama);

  //salida digital cambia o low e o high, son ao reves
  if (llama) { 
    digitalWrite(Led, LOW); // Si hay llama, enciende el LED
    Serial.println("Hay llamas ");
    sendBoolToInfluxDB("Llama", true);
  } else {
    digitalWrite(Led, HIGH); // No hay llama
    Serial.println("No hay llamas");
    sendBoolToInfluxDB("Llama", false);
  }

  // Mostrar el valor analógico en el Serial
  Serial.print("Valor analógico: ");
  Serial.println(valAnalogico);

  // Enviar los datos a Telegram
  sendMessageToTelegram("Llamas: " + String(llama));

  // Enviar los datos a InfluxDB
  

  delay(5000);
}