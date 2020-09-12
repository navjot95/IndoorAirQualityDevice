
/****************************************************************************

  Source file for IAQ utilities functions

 ****************************************************************************/
#include "IAQ_util.h"
#include <esp32-hal-adc.h>
#include <pins_arduino.h>

#define BAT_PIN A13
#define BAT_OFFSET 350


void updateRunAvg(runAvg_t *runAvgValues, uint16_t newSensorVal)
{
  runAvgValues->runAvgSum += newSensorVal;
  runAvgValues->runAvgSum -= runAvgValues->buff[runAvgValues->oldestIdx]; 
  runAvgValues->buff[runAvgValues->oldestIdx] = newSensorVal; 
  (runAvgValues->oldestIdx)++; 
  (runAvgValues->oldestIdx) %= RUN_AVG_BUFFER_LEN; 
}


uint8_t getCurrTime(char *str, uint8_t len)
{
  time_t now;
  struct tm tmInfo;
  time(&now);
  localtime_r(&now, &tmInfo);

  uint8_t hr = (tmInfo.tm_hour % 12) ? (tmInfo.tm_hour % 12) : 12; 
  const char *dn = (tmInfo.tm_hour < 12) ? "AM" : "PM"; 
  uint8_t returnVal = snprintf(str, len, "%d/%d %d:%02d %s", tmInfo.tm_mon+1, tmInfo.tm_mday, hr, tmInfo.tm_min, dn);
  if(returnVal+1 <= len)  // snprintf returns how many chars would be written (not including null char) if the buffer is big enough
    return 1; 
  else
    return -1; 
} 

bool isTimeSynced()
{
  // time is initialized to 0 at power reset
  time_t now;
  struct tm tmInfo;
  time(&now);
  localtime_r(&now, &tmInfo);

  if(tmInfo.tm_year > (2019-1900))
    return true;
  else 
    return false;
}


uint8_t getBatPerct()
{
  uint16_t batV = getBatVolt(); 
  if(batV < 3600)
  {
    return 5; 
  } else if (batV < 3700)
  {
    return 10; 
  } else if (batV < 3800)
  {
    return 30; 
  } else if (batV < 3900)
  {
    return 60; 
  } else if (batV < 4000)
  {
    return 70; 
  } else if (batV < 4100)
  {
    return 85; 
  } else if (batV < 4200)
  {
    return 95; 
  } else 
  {
    return 100; 
  }
}

uint16_t getBatVolt()
{
  static runAvg_t batRunAvg = {.runAvgSum=32000, .buff={4000, 4000, 4000, 4000, 4000, 4000, 4000, 4000}, .oldestIdx=0};
  updateRunAvg(&batRunAvg, (analogRead(BAT_PIN) * 2) - BAT_OFFSET);

  return round((float)batRunAvg.runAvgSum / (float)RUN_AVG_BUFFER_LEN);; 
}

