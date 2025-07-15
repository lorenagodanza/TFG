#include <Wire.h>
#include <ESP8266WiFi.h>
#include "bd_bot.h"

// Dirección del MPU6050
const int MPU_addr = 0x68;

int16_t AcX, AcY, AcZ;

// Valores de aceleración en g
float ax, ay, az;

const float FALL_THRESHOLD = 2.5; // Umbral de aceleración para detectar caída (en g)
const float POST_FALL_THRESHOLD = 0.5; // Umbral para detectar si la persona está inmovil o si se ha levantado
const unsigned long FALL_TIME_WINDOW = 10000; // tiempo en el que observamos si la persona se levanta o no 

bool fallDetected = false;
unsigned long fallStartTime = 0;

void setup() {
  Serial.begin(115200);
  setupBotAndWiFi();
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // registro gestion energia (PWR_MGMT_1)
  Wire.write(0);     // se escribe 0 en el registro 0x6B (activa el sensor)
  Wire.endTransmission(true);
  
  // Configurar el rango del acelerómetro a ±8g
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x1C);  // registro de configuracion de aceleracion (ACCEL_CONFIG)
  //seleccionamos 8g  
  Wire.write(0x10);  // 8 en binario -> 0001 0000. En hexadecimal es 10
  Wire.endTransmission(true);
  
  Serial.println("Sistema de detección de caídas inicializado");
  delay(1000);
}

void loop() {
  readMPU6050();
  
  float acceleration = sqrt(ax*ax + ay*ay + az*az);
  
  if (!fallDetected && acceleration > FALL_THRESHOLD) {
    fallDetected = true;
    fallStartTime = millis();
    Serial.println("¡Impacto detectado! Monitoreando estado...");
  }
  
  if (fallDetected) {
    unsigned long elapsedTime = millis() - fallStartTime;
    
    if (elapsedTime > 5000 && elapsedTime < FALL_TIME_WINDOW) {
      // Solo analizamos movimiento después de 5s
      if (acceleration > 1.0 + POST_FALL_THRESHOLD || acceleration < 1.0 - POST_FALL_THRESHOLD) {
        Serial.println("Movimiento detectado después del impacto. Cancelando alerta.");
        sendBoolToInfluxDB("Caida", false);
        fallDetected = false;
      }
    }
    else if (elapsedTime >= FALL_TIME_WINDOW) {
      // Han pasado los 10 segundos, comprobar inmovilidad
      if (acceleration > 1.0 - POST_FALL_THRESHOLD && acceleration < 1.0 + POST_FALL_THRESHOLD) {
        Serial.println("¡ALERTA! Posible caída detectada. La persona no se mueve.");
        sendMessageToTelegram(String("Se ha detectado una caída"));
        sendBoolToInfluxDB("Caida", true);
        fallDetected = false;
      } else {
        Serial.println("Falsa alarma - movimiento detectado después del impacto.");
        sendBoolToInfluxDB("Caida", false);
        fallDetected = false;
      }
    }
  }
  
  delay(100); 
}

void readMPU6050() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  //registro en el que empiezan los datos(ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)MPU_addr, (size_t)6, true);  // al pedir 6 bytes pedimos (X,Y,Z)
  
  // Leer los datos del acelerómetro en orden
  // desplazar á esquerda para ter num de 16 bits e facer OR
  AcX = Wire.read() << 8 | Wire.read();  
  AcY = Wire.read() << 8 | Wire.read();  
  AcZ = Wire.read() << 8 | Wire.read();  
  
  // Convertir a valores en g (úsase 8g)
  ax = AcX / 4096.0;
  ay = AcY / 4096.0;
  az = AcZ / 4096.0;
}