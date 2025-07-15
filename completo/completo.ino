#include <Wire.h>
#include <DHT.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "bd_bot.h"
#include "BuzzerControl.h"

// Configuración de pines
#define DHTPIN D5          
#define DHTTYPE DHT11
const int MQ135_DIGITAL = D6; 
const int FLAME_SENSOR = D7;  
const int WATER_LEVEL_SENSOR = A0; 

// Dirección del MPU6050
const int MPU_addr = 0x68;

DHT dht(DHTPIN, DHTTYPE);
MAX30105 particleSensor;

// para MAX30102
uint32_t irBuffer[100];
uint32_t redBuffer[100];
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

const int MIN_HR = 30;
const int MAX_HR = 220;
const int MIN_SPO2 = 70;
const int MAX_SPO2 = 100;

// para MPU6050
int16_t AcX, AcY, AcZ;
float ax, ay, az;
const float FALL_THRESHOLD = 2.5;
const float POST_FALL_THRESHOLD = 0.5;
const unsigned long FALL_TIME_WINDOW = 10000;
bool fallDetected = false;
unsigned long fallStartTime = 0;
unsigned long lastMPURead = 0;
const unsigned long MPU_READ_INTERVAL = 50; // 50ms = 20Hz

const int MOVING_AVG_WINDOW = 5;
int hrBuffer[MOVING_AVG_WINDOW] = {0};
int spo2Buffer[MOVING_AVG_WINDOW] = {0};
int bufferIndex = 0;
bool bufferInitialized = false;

// Variables para temporización
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_READ_INTERVAL = 5000; // 5 segundos

void setup() {
  Serial.begin(115200);
  Wire.begin(D2, D1);
  
  // Inicializar WiFi y bot
  setupBotAndWiFi();
  
  // Configurar pines
  pinMode(MQ135_DIGITAL, INPUT);
  pinMode(FLAME_SENSOR, INPUT);
  pinMode(WATER_LEVEL_SENSOR, INPUT);
  setupBuzzer(D0);
  
  // Inicializar DHT11
  dht.begin();

  // Inicializar MPU6050
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1
  Wire.write(0);     // Activa el sensor
  Wire.endTransmission(true);
  
  // Configurar el rango del acelerómetro a ±8g
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x1C);  // ACCEL_CONFIG
  Wire.write(0x10);  // ±8g
  Wire.endTransmission(true);

  // Inicializar MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("Error en el sensor MAX30102");
    while (1);
  }
  
  // Configuración MAX30102
  particleSensor.setup(80, 4, 2, 200, 411, 4096);
  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeIR(0x1F);

  Serial.println("Sensores inicializados");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Lectura de MPU6050 con intervalo fijo
  if (currentMillis - lastMPURead >= MPU_READ_INTERVAL) {
    lastMPURead = currentMillis;
    readMPU6050();
    checkFallDetection();
  }
  
  // Lectura de otros sensores cada 5 segundos
  if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = currentMillis;
    readDHT();
    readMQ135();
    readFlameSensor();
    readWaterLevel();
  }
  
  // Leer MAX30102 continuamente
  readMAX30102();
  
  checkWiFiAndProcessBuffer();
}

void readDHT() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    Serial.println("Error al leer el sensor DHT!");
    return;
  }
 
  Serial.print("Humedad: ");
  Serial.print(h);
  Serial.print("% | Temperatura: ");
  Serial.print(t);
  Serial.println("°C");
  
  sendToInfluxDB("humidity", h);
  sendToInfluxDB("temperature", t);

  // Enviar alertas
  if (t > 40) {
    sendMessageToTelegram("Temperatura muy alta! " + String(t) + "°C");
    soundAlarmForDuration(10000);
  } else if (t < 10) {
    //sendMessageToTelegram("Temperatura muy baja! " + String(t) + "°C");
    //soundAlarmForDuration(10000);
  }

  if (h > 85) {
    sendMessageToTelegram("Humedad muy alta! " + String(h) + "%");
    soundAlarmForDuration(10000);
  } else if (h < 20) {
    //sendMessageToTelegram("Humedad muy baja! " + String(h) + "%");
    //soundAlarmForDuration(10000);
  }
}

void readMQ135() {
  int valDigital = digitalRead(MQ135_DIGITAL);
  
  if(valDigital) {
    Serial.println("No se detecta gas");
    sendBoolToInfluxDB("Gas", false);
  } else {
    Serial.println("Peligro de gas detectado!");
    sendMessageToTelegram("Alerta! Peligro de gas");
    soundAlarmForDuration(10000);
    sendBoolToInfluxDB("Gas", true);
  }
}

void readFlameSensor() {
  bool flameDetected = (digitalRead(FLAME_SENSOR) == HIGH); // HIGH significa llama detectada
  
  if (flameDetected) {
    Serial.println("¡Llamas detectadas!");
    sendMessageToTelegram("¡ALERTA! Se han detectado llamas");
    soundAlarmForDuration(10000);
    sendBoolToInfluxDB("Llama", true);
  } else {
    Serial.println("No se detectan llamas");
    sendBoolToInfluxDB("Llama", false);
  }
}

void readWaterLevel() {
  int value = analogRead(WATER_LEVEL_SENSOR);
  Serial.print("Nivel de agua: ");
  Serial.println(value);
  
  sendToInfluxDB("WaterLevel", value);
  
  if(value > 600) {
    sendMessageToTelegram("Alerta: Nivel alto en tanque!!");
    soundAlarmForDuration(10000);
  }
}

void readMAX30102() {
  static unsigned long lastCheck = 0;
  
  if (millis() - lastCheck >= 1000) { 
    lastCheck = millis();

    // Lectura de 100 muestras
    for (int i = 0; i < 100; i++) {
      while (!particleSensor.available()) {
        particleSensor.check();
      }
      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample();
    }

    // Procesamiento
    Serial.println("MAX30102: Procesando datos para HR y SpO2...");
    maxim_heart_rate_and_oxygen_saturation(irBuffer, 100, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
    Serial.println("MAX30102: Procesamiento completado");
    
    bool validReading = true;
    
    // Validación HR
    if (heartRate < MIN_HR || heartRate > MAX_HR || validHeartRate == 0) {
      validHeartRate = 0;
      validReading = false;
      Serial.println("MAX30102: Lectura de HR no válida");
    }
    
    // Validación SpO2
    if (spo2 < MIN_SPO2 || spo2 > MAX_SPO2 || validSPO2 == 0) {
      validSPO2 = 0;
      validReading = false;
      Serial.println("MAX30102: Lectura de SpO2 no válida");
    }

    if (validReading) {
      // Aplicar media móvil solo si lectura válida
      Serial.print("HR: ");
Serial.print(heartRate);
Serial.print(" bpm | SpO2: ");
Serial.print(spo2);
Serial.println(" %");
      hrBuffer[bufferIndex] = heartRate;
      spo2Buffer[bufferIndex] = spo2;
      bufferIndex = (bufferIndex + 1) % MOVING_AVG_WINDOW;
      
      // Solo considerar el buffer inicializado ventana completa de valores válidos
      if (!bufferInitialized && bufferIndex == 0) {
        bufferInitialized = true;
        Serial.println("MAX30102: Buffer de media móvil inicializado");
      }

      if (bufferInitialized) {
        int sumHR = 0, sumSpO2 = 0;
        
        for (int i = 0; i < MOVING_AVG_WINDOW; i++) {
          sumHR += hrBuffer[i];
          sumSpO2 += spo2Buffer[i];
        }
        
        int avgHR = sumHR / MOVING_AVG_WINDOW;
        int avgSpO2 = sumSpO2 / MOVING_AVG_WINDOW;

        // Mostrar resultados
        Serial.print("MAX30102: HR promedio: ");
        Serial.print(avgHR);
        Serial.print(" bpm | SpO2 promedio: ");
        Serial.print(avgSpO2);
        Serial.println("%");
        
        // Enviar a InfluxDB (esto se hará cada 5 segundos con los otros sensores)
        static unsigned long lastSend = 0;
        if (millis() - lastSend >= SENSOR_READ_INTERVAL) {
          lastSend = millis();
          sendToInfluxDB("HR", avgHR);
          sendToInfluxDB("SpO2", avgSpO2);
          
          // Enviar alertas
          if (avgHR > 140) {
            sendMessageToTelegram("¡ALERTA! Frecuencia cardiaca media mayor a 140 bpm.");
            soundAlarmForDuration(10000);          
          }
          if (avgHR < 40) {
            sendMessageToTelegram("¡ALERTA! Frecuencia cardiaca media menor a 40 bpm.");
            soundAlarmForDuration(10000);
          }
          if (avgSpO2 < 85) {
            sendMessageToTelegram("¡ALERTA! SpO2 medio menor a 85%.");
            soundAlarmForDuration(10000);
          }
        }
      } else {
        Serial.print("MAX30102: Inicializando buffer... ");
        Serial.print(bufferIndex);
        Serial.println("/5 mediciones válidas");
      }
    } else {
      Serial.println("MAX30102: Ruido detectado - descartando medición");
    }
  }
}

void readMPU6050() {
  Serial.println("MPU6050: Iniciando lectura de acelerómetro...");
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  //registro en el que empiezan los datos(ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)MPU_addr, (size_t)6, true);  // al pedir 6 bytes pedimos (X,Y,Z)
  Serial.println("MPU6050: Datos solicitados via I2C");
  
  // Leer los datos del acelerómetro en orden
  AcX = Wire.read() << 8 | Wire.read();  
  AcY = Wire.read() << 8 | Wire.read();  
  AcZ = Wire.read() << 8 | Wire.read();  
  Serial.println("MPU6050: Datos leídos del acelerómetro");
  
  // Convertir a valores en g (úsase 8g)
  ax = AcX / 4096.0;
  ay = AcY / 4096.0;
  az = AcZ / 4096.0;
}

void checkFallDetection() {
  float acceleration = sqrt(ax*ax + ay*ay + az*az);
  
  if (!fallDetected && acceleration > FALL_THRESHOLD) {
    fallDetected = true;
    fallStartTime = millis();
    Serial.println("MPU6050: ¡Impacto detectado! Monitoreando estado...");
    sendMessageToTelegram(String("Impacto detectado, monitorizando..."));
  }
  
  if (fallDetected) {
    unsigned long elapsedTime = millis() - fallStartTime;
    
    if (elapsedTime > 5000 && elapsedTime < FALL_TIME_WINDOW) {
      // Solo analizamos movimiento después de 5s
      if (acceleration > 1.0 + POST_FALL_THRESHOLD || acceleration < 1.0 - POST_FALL_THRESHOLD) {
        Serial.println("MPU6050: Movimiento detectado después del impacto. Cancelando alerta.");
        sendMessageToTelegram(String("cancelando alerta 1"));
        sendBoolToInfluxDB("Caida", false);
        fallDetected = false;
      }
    }
    else if (elapsedTime >= FALL_TIME_WINDOW) {
      // Han pasado los 10 segundos, comprobar inmovilidad
      if (acceleration > 1.0 - POST_FALL_THRESHOLD && acceleration < 1.0 + POST_FALL_THRESHOLD) {
        Serial.println("MPU6050: ¡ALERTA! Posible caída detectada. La persona no se mueve.");
        sendMessageToTelegram(String("Se ha detectado una caída"));
        soundAlarmForDuration(10000);
        sendBoolToInfluxDB("Caida", true);
        fallDetected = false;
      } else {
        Serial.println("MPU6050: Falsa alarma - movimiento detectado después del impacto.");
        sendMessageToTelegram(String("cancelando alerta 2"));
        sendBoolToInfluxDB("Caida", false);
        fallDetected = false;
      }
    }
  }
}