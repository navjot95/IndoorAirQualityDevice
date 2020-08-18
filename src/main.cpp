#include <Arduino.h>
#include "ES_framework.h"

ES_Return_t ES_returnVal; 


void setup() {
  Serial.begin(115200); 
  while(!Serial){;}

  TimerRate_t Rate = ES_Timer_RATE_1mS;  // each tick in sw timer will decrement every 1ms
  ES_returnVal = ES_Initialize(Rate); 
  if(ES_returnVal != Success)
  {
    // digitalWrite(LED_PIN, HIGH);  // turn light on for debugging 
    Serial.printf("Err: %i\n", ES_returnVal);
    Serial.println("******Stoping code*******");
  } 

}


void loop(){
  static bool sentFlag = false; 
  if(ES_returnVal == Success)
    ES_returnVal = ES_Run(); 

  if(ES_returnVal != Success && !sentFlag)
  {
    // digitalWrite(LED_PIN, HIGH);  // turn light on for debugging 
    Serial.printf("Err: %i\n", ES_returnVal);
    Serial.println("******Stopping code*******");
    sentFlag = true; 
  }
}