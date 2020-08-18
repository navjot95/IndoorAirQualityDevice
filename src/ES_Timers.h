/****************************************************************************
 Module
         ES_Timers.h

 Revision
         1.0.1

 Description
         Header File for the Timer Module.
         This module has been taken from Ed Carryer's Events & Services Framework

 Notes
****************************************************************************/

#ifndef ES_Timers_H
#define ES_Timers_H

#include "ES_Port.h"

typedef enum
{
  ES_Timer_ERR        = -1,
  ES_Timer_ACTIVE     = 1,
  ES_Timer_OK         = 0,
  ES_Timer_NOT_ACTIVE = 0
}ES_TimerReturn_t;


// These are to be used by the services 
ES_TimerReturn_t ES_Timer_InitTimer(uint8_t Num, uint32_t NewTime);
ES_TimerReturn_t ES_Timer_SetTimer(uint8_t Num, uint32_t NewTime);
ES_TimerReturn_t ES_Timer_StartTimer(uint8_t Num);
ES_TimerReturn_t ES_Timer_StopTimer(uint8_t Num);
uint16_t ES_Timer_GetTime(void);

// These two are used by the framework
void ES_Timer_Init(TimerRate_t Rate);
void ES_Timer_Tick_Resp(void);


#endif   /* ES_Timers_H */
/*------------------------------ End of file ------------------------------*/

