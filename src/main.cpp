
#include "ES_framework.h"
#include <stdint.h>
#include <stdbool.h>
#include <Arduino.h>

// #define PIN_DEBUG
#ifdef PIN_DEBUG
    #define DEBUG_PIN 21
    bool pinState = false; 
#endif

ES_Return_t ES_returnVal; 

void setup() {
  setCpuFrequencyMhz(80);  // 80Mhz is the lowest for bluetooth and Wifi. Otherwise 10Mhz 
  Serial.begin(115200); 
  while(!Serial){;}

  TimerRate_t Rate = ES_Timer_RATE_1mS;  // each tick in sw timer will decrement every 1ms
  ES_returnVal = ES_Initialize(Rate); 
  if(ES_returnVal != Success)
  {
    // digitalWrite(BUILTIN_LED, HIGH);  // turn light on for debugging 
    Serial.printf("Err: %i\n", ES_returnVal);
    Serial.println("******Stoping code*******");
  } 

  #ifdef PIN_DEBUG
    pinMode(DEBUG_PIN, OUTPUT); 
  #endif
}


void loop(){
  #ifdef PIN_DEBUG
    pinState ^= 0x01; 
    digitalWrite(DEBUG_PIN, pinState); 
  #endif
  static bool sentFlag = false; 
  if(ES_returnVal == Success)
    ES_returnVal = ES_Run(); 

  if(ES_returnVal != Success && !sentFlag)
  {
    // digitalWrite(BUILTIN_LED, HIGH);  // turn light on for debugging 
    Serial.printf("Err: %i\n", ES_returnVal);
    Serial.println("******Stopping code*******");
    sentFlag = true; 
  }
}