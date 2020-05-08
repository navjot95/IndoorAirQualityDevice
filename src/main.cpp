#include <Arduino.h>
#include "ES_framework.h"

ES_Return_t ES_returnVal; 
void setup() {
  Serial.begin(115200); 
  while(!Serial){;}

  ES_returnVal = ES_Initialize(); 
  if(ES_returnVal != Success)
  {
    // digitalWrite(LED_PIN, HIGH);  // turn light on for debugging 
    Serial.printf("Err: %i\n", ES_returnVal);
    Serial.println("******Stoping code*******");
  } 

}


void loop() {
  static bool sentFlag = false; 
  if(ES_returnVal == Success)
    ES_returnVal = ES_Run(); 

  if(ES_returnVal != Success && !sentFlag)
  {
    // digitalWrite(LED_PIN, HIGH);  // turn light on for debugging 
    Serial.printf("Err: %i\n", ES_returnVal);
    Serial.println("******Stoping code*******");
    sentFlag = true; 
  }
}