/****************************************************************************
 Module
     ES_Port.h
 Description
     header file to collect all of the hardware/platform dependent info for
     a particular port of the ES framework. Currently configured to be used 
     with the Arduino API
 Notes
*****************************************************************************/

#ifndef ES_PORT_H
#define ES_PORT_H

// pull in the hardware header files that we need

#include <stdio.h>
#include <stdint.h>


/* 
   These values will be used to set the hw timer alarm. 
   Values assume a 80MHz clock rate with pre-scaler val 2, so sysTick counter
   will increment every 1us.
 */
typedef enum
{
  ES_Timer_RATE_500uS = 20000UL, // -1UL,
  ES_Timer_RATE_1mS   = 40000UL, // -1UL,
  ES_Timer_RATE_2mS   = 80000UL, // -1UL
}TimerRate_t;


// prototypes for the hardware specific routines
void _HW_Timer_Init(TimerRate_t Rate);
bool _HW_Process_Pending_Ints(void);
uint32_t _HW_GetTickCount(void);
void ConsoleInit(void);



#endif
