
/****************************************************************************

  Header file for IAQ utilities functions

 ****************************************************************************/

#ifndef IAQUTIL_H
#define IAQUTIL_H

#include <rom/rtc.h>
#include <time.h>

#define PWR_RESET 1

#ifdef DEBUG
#define DEBUG_TEST 1
#else
#define DEBUG_TEST 0
#endif

#define IAQ_PRINTF(fmt, ...) \
        do { if (DEBUG_TEST) Serial.printf("DEBUG %s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, __VA_ARGS__); } while (0)

typedef enum
{
  AUTO_MODE=0, 
  STREAM_MODE, 
  NO_MODE  // Used to remove label for screen 
} IAQmode_t; 


// prints current time into char *str in format: mm/dd hh:mm AM/PM
// returns 1 on sucess and -1 if buffer is not big enough
uint8_t getCurrTime(char *str, uint8_t len);
bool isTimeSynced(); 

uint16_t getBatVolt(); 
uint8_t getBatPerct();

#endif 



