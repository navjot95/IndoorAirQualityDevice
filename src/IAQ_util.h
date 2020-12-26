
/****************************************************************************

  Header file for IAQ utilities functions

 ****************************************************************************/

#ifndef IAQUTIL_H
#define IAQUTIL_H

#include <HardwareSerial.h>
#include <rom/rtc.h>
#include <time.h>

#define RUN_AVG_BUFFER_LEN 8  // size of running average buffer MAX VALUE: 255

// #define IAQ_DEBUG_ENABLE  // Uncomment this to enable debugging printfs
#ifdef IAQ_DEBUG_ENABLE
#define DEBUG_CHECK 1
#else
#define DEBUG_CHECK 0
#endif

#define IAQ_PRINTF(fmt, ...) do{ if (DEBUG_CHECK) Serial.printf(fmt, ## __VA_ARGS__); } while (0)
#define IAQ_PRINTFV(fmt, ...) do{ if (DEBUG_CHECK) Serial.printf("[D] %s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ## __VA_ARGS__); } while (0)



typedef enum
{
  AUTO_MODE=0, 
  STREAM_MODE, 
  NO_MODE  // Used to remove label for screen 
} IAQmode_t; 

typedef struct
{
    uint32_t runAvgSum;
    uint16_t buff[RUN_AVG_BUFFER_LEN]; 
    uint8_t oldestIdx;  
}runAvg_t; 


void updateRunAvg(runAvg_t *runAvgValues, uint16_t newSensorVal);

// prints current time into char *str in format: mm/dd hh:mm AM/PM
// returns 1 on sucess and -1 if buffer is not big enough
uint8_t getCurrTime(char * str, uint8_t len, time_t * timeUsed);

// same as getCurrTime() but uses provided time rather than current time 
uint8_t convertRawTime(const time_t givenTime, char * str, uint8_t len, time_t * timeUsed);
bool isTimeSynced(); 

uint16_t getBatVolt(); 
uint8_t getBatPerct();

#endif 



