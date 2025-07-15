#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "bd_bot.h"

MAX30105 particleSensor;
//3V
// SCL -> D1
// SDA -> D2

// Buffers para datos
uint32_t irBuffer[100];
uint32_t redBuffer[100];

// Variables de medición
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

// Configuración de filtrado
const int MIN_HR = 30;       
const int MAX_HR = 220;       
const int MIN_SPO2 = 70;     
const int MAX_SPO2 = 100;     


// Configuración media móvil
const int MOVING_AVG_WINDOW = 5;
int hrBuffer[MOVING_AVG_WINDOW] = {0};
int spo2Buffer[MOVING_AVG_WINDOW] = {0};
int bufferIndex = 0;
bool bufferInitialized = false;


void setup() {
  Serial.begin(115200);
  
  setupBotAndWiFi();  

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("Error en el sensor. Verifica conexiones.");
    while (1);
  }

  // Configuración sensor
  byte ledBrightness = 80; //intensidad de LED
  byte sampleAverage = 4; // promedio de muestras
  byte ledMode = 2; // modo LED (rojo + IR)
  int sampleRate = 400; // tasa de muestreo
  int pulseWidth = 411; //ancho de pulso LED
  int adcRange = 4096; //rango adc

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeIR(0x1F);
}

void loop() {
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
    maxim_heart_rate_and_oxygen_saturation(irBuffer, 100, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

    bool validReading = true;
    
    // Validación HR
    if (heartRate < MIN_HR || heartRate > MAX_HR || validHeartRate == 0) {
      validHeartRate = 0;
      validReading = false;
    }
    
    // Validación SpO2
    if (spo2 < MIN_SPO2 || spo2 > MAX_SPO2 || validSPO2 == 0) {
      validSPO2 = 0;
      validReading = false;
    }

    if (validReading) {
      // Aplicar media móvil solo si lectura válida
      hrBuffer[bufferIndex] = heartRate;
      spo2Buffer[bufferIndex] = spo2;
      bufferIndex = (bufferIndex + 1) % MOVING_AVG_WINDOW;
      
      // Solo considerar el buffer inicializado ventana completa de valores válidos
      if (!bufferInitialized && bufferIndex == 0) {
        bufferInitialized = true;
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
        Serial.print("HR: ");
        Serial.print(avgHR);
        Serial.print(" bpm | SpO2: ");
        Serial.print(avgSpO2);
        Serial.println("%");

	sendToInfluxDB("HR", avgHR);
        sendToInfluxDB("SpO2", avgSpO2);

        // Enviar alertas
        if (avgHR > 140) {
          sendMessageToTelegram("¡ALERTA! Frecuencia cardiaca media mayor a 140 bpm.");
        }
        if (avgHR < 40) {
          sendMessageToTelegram("¡ALERTA! Frecuencia cardiaca media menor a 40 bpm.");
        }
        if (avgSpO2 < 85) {
          sendMessageToTelegram("¡ALERTA! SpO2 medio menor a 85%.");
        }
      } else {
        Serial.print("Inicializando buffer... ");
        Serial.print(bufferIndex);
        Serial.println("/5 mediciones válidas");
      }
    } else {
      Serial.println("Ruido detectado - descartando medición");
      }
    }
  }
