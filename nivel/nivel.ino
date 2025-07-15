#include "bd_bot.h"
int adc_id=A0;
int HistoryValue=0;
char printBuffer[128];

void setup(){
  Serial.begin(115200);
  setupBotAndWiFi();
}

void loop(){
  int value = analogRead(adc_id);
  sendMessageToTelegram("Water Level: " + String(value));
  sendToInfluxDB("WaterLevel", value);
  if(((HistoryValue>=value)&&((HistoryValue-value)>10))||((HistoryValue<value)&&((value-HistoryValue)>10))){
    sprintf(printBuffer,"ADC%d level is %d\n",adc_id,value);
    Serial.print(printBuffer);
    HistoryValue=value;
    
  }
  
}
