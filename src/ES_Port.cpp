/****************************************************************************/
/*
 Module
   ES_Port.c

 Revision
   1.0.1

 Description
   This is the sample file to demonstrate adding the hardware specific
   functions to the Events & Services Framework. This code is currently set-up
   to be used with an Arduino ESP32. 

 Notes
*/
 /***************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <esp32-hal-timer.h>

#include "ES_Port.h"
#include "ES_Timers.h"

// #define SYS_TICK_DEBUG
#ifdef SYS_TICK_DEBUG
    #define DEBUG_PIN 21
    bool pinState = false; 
#endif

// TickCount is used to track the number of timer ints that have occurred
// since the last check. It should really never be more than 1, but just to
// be sure, we increment it in the interrupt response rather than simply
// setting a flag. Using this variable and checking approach we remove the
// need to post events from the interrupt response routine. This is necessary
// for compilers like HTC for the midrange PICs which do not produce re-entrant
// code so cannot post directly to the queues from within the interrupt resp.
static volatile uint8_t TickCount;

// Global tick count to monitor number of SysTick Interrupts
// make uint16_t on 8 and 16 bit processors so to not overly burden them
static volatile uint32_t SysTickCounter = 0;

// need mux in order to synchronize between main loop and timer ISR
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;


/****************************************************************************
 Function
     SysTickIntHandler
 Parameters
     none
 Returns
     None.
 Description
     interrupt response routine for the tick interrupt that will allow the
     framework timers to run.
 Notes
     this does not actually post events
     but simply sets a flag to indicate that the interrupt has occurred.
     the framework response is handled below in _HW_Process_Pending_Ints. 
     Loop() runs at period of ~3us. 

****************************************************************************/
void IRAM_ATTR SysTickIntHandler(void)
{
    portENTER_CRITICAL_ISR(&timerMux);
    /* Interrupt automatically cleared by hardware */
    ++TickCount;          /* flag that it occurred and needs a response */
    ++SysTickCounter;     // keep the free running time going
    portEXIT_CRITICAL(&timerMux);

    #ifdef SYS_TICK_DEBUG
    pinState ^= 0x01; 
    digitalWrite(DEBUG_PIN, pinState); 
    #endif
}

/****************************************************************************
 Function
     _HW_Timer_Init
 Parameters
     unsigned char Rate set to one of the TMR_RATE_XX values to set the
     Tick rate
 Returns
     None.
 Description
     Initializes a hardware timer to go off at provided Rate. When timer goes off, 
     its handler, SysTickIntHandler, is called.  
 Notes
     modify as required to port to other timer hardware

****************************************************************************/
void _HW_Timer_Init(TimerRate_t Rate)
{   
    #ifdef SYS_TICK_DEBUG
    pinMode(DEBUG_PIN, OUTPUT); 
    #endif

    // timer will increment the 64-bit counter at 40MHz. 2 is lowest val for pre-scaler 
    hw_timer_t *timer = timerBegin(0, 2, true); //using hw timer 0, prescaler of 0, count up

    timerAttachInterrupt(timer, &SysTickIntHandler, true);

    timerAlarmWrite(timer, Rate, true); // alarm will go off after Rate number of counts

    timerAlarmEnable(timer); // timer is on now
    
}


/****************************************************************************
 Function
    _HW_GetTickCount()
 Parameters
    none
 Returns
    uint16_t   count of number of system ticks that have occurred.
 Description
    wrapper for access to SysTickCounter, needed to move increment of tick
    counter to this module to keep the timer ticking during blocking code
 Notes

 Author
    Ed Carryer, 10/27/14 13:55
****************************************************************************/
uint32_t _HW_GetTickCount(void)
{
  return SysTickCounter;
}


/****************************************************************************
 Function
     _HW_Process_Pending_Ints
 Parameters
     none
 Returns
     always true.
 Description
     processes any pending interrupts (actually the hardware interrupt already
     occurred and simply set a flag to tell this routine to execute the non-
     hardware response)
 Notes
     While this routine technically does not need a return value, we always
     return true so that it can be used in the conditional while() loop in
     ES_Run. This way the test for pending interrupts get processed after every
     run function is called and even when there are no queues with events.
     This routine could be expanded to process any other interrupt sources
     that you would like to use to post events to the framework services.
 Author
     J. Edward Carryer, 08/13/13 13:27
****************************************************************************/
bool _HW_Process_Pending_Ints(void)
{
  while (TickCount > 0)
  {
    /* call the framework tick response to actually run the timers */
    ES_Timer_Tick_Resp();  // With 40MHz clock rate, and alarm value of 40,000-1, this is called every 1ms

    portENTER_CRITICAL_ISR(&timerMux);
    TickCount--;
    portEXIT_CRITICAL(&timerMux);
  }
  return true;  // always return true to allow loop test in ES_Run to proceed
}

/****************************************************************************
 Function
     ConsoleInit
 Parameters
     none
 Returns
     none.
 Description
  TBD
 Notes
  TBD
 ****************************************************************************/
void ConsoleInit(void)
{
  //initialize the clock and UART if needed 
  return;
}


