#include "BuzzerControl.h"
#include <Arduino.h>  // Necesario para pinMode, digitalWrite, delay

int buzzerPin;  // Variable global para almacenar el pin del buzzer

// Configura el buzzer
void setupBuzzer(int pin) {
    buzzerPin = pin;  // Guarda el pin del buzzer
    pinMode(buzzerPin, OUTPUT);  // Configura el pin como salida
}

// Hace sonar el buzzer con un retraso específico
void beep(int delayTime, int times) {
    for (int i = 0; i < times; i++) {
        digitalWrite(buzzerPin, HIGH);   // Activa el buzzer
        delay(delayTime);                // Espera por el tiempo dado
        digitalWrite(buzzerPin, LOW);    // Desactiva el buzzer
        delay(delayTime);                // Espera por el tiempo dado
    }
}

// Suena la alarma durante un tiempo específico (en milisegundos)
void soundAlarmForDuration(unsigned long duration) {
    unsigned long startTime = millis();  // Guarda el tiempo de inicio
    
    // Mientras no haya pasado el tiempo deseado, sigue sonando
    while (millis() - startTime < duration) {
        beep(1, 100);  // Beep corto (1ms) 100 veces
        beep(4, 100);  // Beep más largo (4ms) 100 veces
    }
}
