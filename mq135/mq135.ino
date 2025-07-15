#include "bd_bot.h"
int sensorD0 = D1;
int sensorA0 = A0;
int valDigital;
int valAnalogico;
void setup() {
  Serial.begin(115200);
  setupBotAndWiFi();
  pinMode(sensorD0,INPUT);
}

void loop() {
  // na loxica 
  // 1 -> false
  // 0 -> true
//envianse os valores "normais" falso se non hai gas e true si hai gas
  valDigital = digitalRead(sensorD0);
  valAnalogico = analogRead(sensorA0);
  if(valDigital){
      Serial.println("no gas" );
      Serial.println(valAnalogico);
      sendMessageToTelegram("Gas: " + String("false"));
      sendBoolToInfluxDB("Gas", false);
      
    }else{
      Serial.println("si gas" );
      Serial.println(valAnalogico);
      sendMessageToTelegram("Gas: " + String("true"));
      sendBoolToInfluxDB("Gas", true);
  }
  
  //sendMessageToTelegram("Gas: " + String(valDigital));
  //sendBoolToInfluxDB("Gas: ", valDigital);
  delay(5000);
}
