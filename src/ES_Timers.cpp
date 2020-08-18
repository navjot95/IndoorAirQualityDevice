/****************************************************************************
 Module
     ES_Timers.c

 Description
     This is a module implementing  8 16 bit timers all using the RTI
     timebase. This module has been taken from Ed Carryer's Events & Services Framework

 Notes
     Everything is done in terms of RTI Ticks, which can change from
     application to application.

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
#include "ES_ServicesHeaders.h"
#include "ES_Framework.h"
#include "ES_Event.h"
#include "ES_PostList.h"
#include "ES_Timers.h"
#include "ES_Port.h"

/*--------------------------- External Variables --------------------------*/

/*----------------------------- Module Defines ----------------------------*/
#define BITS_PER_BYTE 8
#define BITS_PER_NYBBLE 4
#define ISOLATE_LS_NYBBLE 0X0F

/*------------------------------ Module Types -----------------------------*/

/*
   the size of Tflag sets the number of timers, uint8 = 8, uint16 = 16 ...)
   to add more timers, you will need to change the data type and modify
   the initialization of TMR_TimerArray and TMR_MaskArray
*/

typedef uint16_t Tflag_t; // create the number of timers as 16, each bit being 1 sw timer

typedef uint32_t Timer_t; // sets size of timers to 32 bits

/*---------------------------- Module Functions ---------------------------*/
uint8_t ES_GetMSBitSet(uint16_t Val2Check);


/*---------------------------- Module Variables ---------------------------*/

/*
  this table is used to go from an unsigned 4bit value to the most significant
  bit that is set in that nybble. It is used in the determination of priorities
  from the Ready variable and in determining active timers in
  the timer interrupt response. Index into the array with (ByteVal-1) to get
  the correct MS Bit num.
*/
uint8_t const Nybble2MSBitNum[15] = {
  0U, 1U, 1U, 2U, 2U, 2U, 2U, 3U, 3U, 3U, 3U, 3U, 3U, 3U, 3U
};

static Timer_t TMR_TimerArray[sizeof(Tflag_t) * BITS_PER_BYTE] =
{
  0x0,
  0x0,
  0x0,
  0x0,
  0x0,
  0x0,
  0x0,
  0x0,
  0x0,
  0x0,
  0x0,
  0x0,
  0x0,
  0x0,
  0x0,
  0x0
};

static Tflag_t TMR_ActiveFlags;

static pPostFunc const Timer2PostFunc[sizeof(Tflag_t) * BITS_PER_BYTE] = 
{
  TIMER0_RESP_FUNC,
  TIMER1_RESP_FUNC,
  TIMER2_RESP_FUNC,
  TIMER3_RESP_FUNC,
  TIMER4_RESP_FUNC,
  TIMER5_RESP_FUNC,
  TIMER6_RESP_FUNC,
  TIMER7_RESP_FUNC,
  TIMER8_RESP_FUNC,
  TIMER9_RESP_FUNC,
  TIMER10_RESP_FUNC,
  TIMER11_RESP_FUNC,
  TIMER12_RESP_FUNC,
  TIMER13_RESP_FUNC,
  TIMER14_RESP_FUNC,
  TIMER15_RESP_FUNC
};

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     ES_Timer_Init
 Parameters
     unsigned char Rate set to one of the TMR_RATE_XX values to set the
     tick rate these are defined by the hardware and placed in ES_Port.h
 Returns
     None.
 Description
     Initializes the timer module by setting up the tick at the requested
    rate
 Notes
     None.
 Author
     J. Edward Carryer, 02/24/97 14:23
****************************************************************************/
void ES_Timer_Init(TimerRate_t Rate)
{
  // call the hardware init routine
  _HW_Timer_Init(Rate);
}

/****************************************************************************
 Function
     ES_Timer_SetTimer
 Parameters
     unsigned char Num, the number of the timer to set.
     unsigned int NewTime, the new time to set on that timer
 Returns
     ES_Timer_ERR if requested timer does not exist or has no service
     ES_Timer_OK  otherwise
 Description
     sets the time for a timer, but does not make it active.
 Notes
     None.
 Author
     J. Edward Carryer, 02/24/97 17:11
****************************************************************************/
ES_TimerReturn_t ES_Timer_SetTimer(uint8_t Num, uint32_t NewTime)
{
  /* tried to set a timer that doesn't exist */
  if ((Num >= ARRAY_SIZE(TMR_TimerArray)) ||
      /* tried to set a timer without a service */
      (Timer2PostFunc[Num] == TIMER_UNUSED) ||
      (NewTime == 0))   /* no time being set */
  {
    return ES_Timer_ERR;
  }
  TMR_TimerArray[Num] = NewTime;
  return ES_Timer_OK;
}

/****************************************************************************
 Function
     ES_Timer_StartTimer
 Parameters
     unsigned char Num the number of the timer to start
 Returns
     ES_Timer_ERR for error ES_Timer_OK for success
 Description
     simply sets the active flag in TMR_ActiveFlags to (re)start a
     stopped timer.
 Notes
     None.
 Author
     J. Edward Carryer, 02/24/97 14:45
****************************************************************************/
ES_TimerReturn_t ES_Timer_StartTimer(uint8_t Num)
{
  /* tried to set a timer that doesn't exist */
  if ((Num >= ARRAY_SIZE(TMR_TimerArray)) ||
      /* tried to set a timer with no time on it */
      (TMR_TimerArray[Num] == 0))
  {
    return ES_Timer_ERR;
  }
  bitSet(TMR_ActiveFlags, Num);  // set timer as active

  return ES_Timer_OK;
}

/****************************************************************************
 Function
     ES_Timer_StopTimer
 Parameters
     unsigned char Num the number of the timer to stop.
 Returns
     ES_Timer_ERR for error (timer doesn't exist) ES_Timer_OK for success.
 Description
     simply clears the bit in TMR_ActiveFlags associated with this
     timer. This will cause it to stop counting.
 Notes
     None.
 Author
     J. Edward Carryer, 02/24/97 14:48
****************************************************************************/
ES_TimerReturn_t ES_Timer_StopTimer(uint8_t Num)
{
  if (Num >= ARRAY_SIZE(TMR_TimerArray))
  {
    return ES_Timer_ERR;    /* tried to set a timer that doesn't exist */
  }
  bitClear(TMR_ActiveFlags, Num);  // set timer as inactive
  return ES_Timer_OK;
}

/****************************************************************************
 Function
     ES_Timer_InitTimer
 Parameters
     unsigned char Num, the number of the timer to start
     unsigned int NewTime, the number of ticks to be counted
 Returns
     ES_Timer_ERR if the requested timer does not exist, ES_Timer_OK otherwise.
 Description
     sets the NewTime into the chosen timer and sets the timer active to
     begin counting.
 Notes
     None.
 Author
     J. Edward Carryer, 02/24/97 14:51
****************************************************************************/
ES_TimerReturn_t ES_Timer_InitTimer(uint8_t Num, uint32_t NewTime)
{
  /* tried to set a timer that doesn't exist */
  if ((Num >= ARRAY_SIZE(TMR_TimerArray)) ||
      /* tried to set a timer without a service */
      (Timer2PostFunc[Num] == TIMER_UNUSED) ||
      /* tried to set a timer without putting any time on it */
      (NewTime == 0))
  {
    return ES_Timer_ERR;
  }
  TMR_TimerArray[Num] = NewTime;
  bitSet(TMR_ActiveFlags, Num);  // set timer as active
  return ES_Timer_OK;
}

/****************************************************************************
 Function
     ES_Timer_GetTime
 Parameters
     None.
 Returns
     the current value of the module variable 'time'
 Description
     Provides the ability to grab a snapshot time as an alternative to using
      the library timers. Can be used to determine how long between 2 events.
 Notes
     this functionality is ancient, though this implementation in the library
     is new.
 Author
     J. Edward Carryer, 06/01/04 08:04
****************************************************************************/
uint16_t ES_Timer_GetTime(void)
{
  return _HW_GetTickCount();
}

/****************************************************************************
 Function
     ES_Timer_Tick_Resp
 Parameters
     None.
 Returns
     None.
 Description
     This is the new Tick response routine to support the timer module.
     It will increment time, to maintain the functionality of the
     GetTime() timer and it will check through the active timers,
     decrementing each active timers count, if the count goes to 0, it
     will post an event to the corresponding SM and clear the active flag to
     prevent further counting.
 Notes
     Called from _Timer_Int_Resp in ES_Port.c.
 Author
     J. Edward Carryer, 02/24/97 15:06
****************************************************************************/
void ES_Timer_Tick_Resp(void)
{
  static Tflag_t  NeedsProcessing;
  static uint8_t  NextTimer2Process;
  static ES_Event_t NewEvent;

  if (TMR_ActiveFlags != 0) /* if !=0 , then at least 1 timer is active */
  {
    // start by getting a list of all the active timers
    NeedsProcessing = TMR_ActiveFlags;
    do
    {
      // find the MSB that is set
      NextTimer2Process = ES_GetMSBitSet(NeedsProcessing);
      /* decrement that timer, check if timed out */
      if (--TMR_TimerArray[NextTimer2Process] == 0)
      {
        NewEvent.EventType  = ES_TIMEOUT;
        NewEvent.EventParam = NextTimer2Process;
        /* post the timeout event to the right Service */
        Timer2PostFunc[NextTimer2Process](NewEvent);
        /* and stop counting */
        bitClear(TMR_ActiveFlags, NextTimer2Process);
      }
      // mark off the active timer that we just processed
      bitClear(NeedsProcessing, NextTimer2Process); 
    } while (NeedsProcessing != 0);
  }
}


/***************************************************************************
 private functions
 ***************************************************************************/
uint8_t ES_GetMSBitSet(uint16_t Val2Check)
{
  int8_t  LoopCntr;
  uint8_t Nybble2Test;
  uint8_t ReturnVal = 128; // this is the error return value

  // loop through the parameter, nybble by nybble
  for (LoopCntr = sizeof(Val2Check) * (BITS_PER_BYTE / BITS_PER_NYBBLE) - 1;
      LoopCntr >= 0; LoopCntr--)
  {
    // move a nybble into the 4 LSB positions for lookup
    Nybble2Test = (uint8_t)
        ((Val2Check >> (uint8_t)(LoopCntr * BITS_PER_NYBBLE)) &
        ISOLATE_LS_NYBBLE);
    if (Nybble2Test != 0)
    {
      // lookup the bit num & adjust for the number of shifts to get there
      ReturnVal = Nybble2MSBitNum[Nybble2Test - 1] +
          (LoopCntr * BITS_PER_NYBBLE);
      break;
    }
  }
  return ReturnVal;
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

