#ifndef BUZZERCONTROL_H
#define BUZZERCONTROL_H

void setupBuzzer(int pin);  // Configura el buzzer
void soundAlarmForDuration(unsigned long duration);  // Suena la alarma durante un tiempo específico
void beep(int delayTime, int times);  // Emite el beep

#endif
