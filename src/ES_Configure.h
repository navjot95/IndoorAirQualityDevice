/****************************************************************************
 Module
     ES_Configure.h
 Description
     This file contains macro definitions that are edited by the user to
     adapt the Events and Services framework to a particular application.
     This is the only "ES_" file you need to edit
 Notes
*****************************************************************************/

#ifndef ES_CONFIGURE_H
#define ES_CONFIGURE_H


/********************** Pin Configuration ***********************************/ 
/****************************************************************************/
#define BUTTON_PIN 26
#define PWR_EN_PIN 12
#define TIMER_TEST_PIN 4
#define BAUD_RATE 115200

#define SHORT_BT_PRESS 0
#define LONG_BT_PRESS 1

/****************************************************************************/
// Name/define the events of interest
// Universal events occupy the lowest entries, followed by user-defined events
typedef enum
{
  ES_NO_EVENT = 0,
  ES_ERROR,                 /* used to indicate an error from the service */
  ES_INIT,                  /* used to transition from initial pseudo-state */
  ES_TIMEOUT,               /* signals that the timer has expired */
  ES_FAIL,                  /* used to indicate something was unsuccessful */ 
  ES_SUCCESS,               /* used to indicate something was successful */ 
  /* User-defined events start here */
  ES_SERIAL,
  ES_SERIAL1,
  ES_SERIAL2,
  ES_I2C,
  ES_HW_BUTTON_EVENT,         /* when physical button pressed (1) or released (0)*/
  ES_SW_BUTTON_PRESS,          /* short button press (0) and long button press (1)*/
  ES_READ_SENSOR,               /* command to send to sensor to read its value(s) */
  COMMSM_SEND,
  SENSORS_READ_EVENT,
  WIFI_CONNECT,
  CLOUD_PUBLISH, 
  CLOUD_UPDATED_EVENT
}ES_EventType_t;


/****************************************************************************/
// add all event checker functions here (comma separated)
// the functions at the beginning of the list are checked first 

//EventCheckerKeyBoard, EventCheckerButton
#define EVENT_CHECKER_LIST EventCheckerCO2, EventCheckerHPM, EventChecker_SVM30, EventCheckerButton



/****************************** Timers **************************************/ 
// These are the definitions for the post functions to be executed when the
// corresponding timer expires. All 16 must be defined. If you are not using
// a timer, then you should use TIMER_UNUSED
// Unlike services, any combination of timers may be used and there is no
// priority in servicing them
#define TIMER_UNUSED ((pPostFunc)0)
#define TIMER0_RESP_FUNC PostCO2Service
#define TIMER1_RESP_FUNC PostHPMService
#define TIMER2_RESP_FUNC PostHPMService
#define TIMER3_RESP_FUNC PostMainService
#define TIMER4_RESP_FUNC PostSVM30Service
#define TIMER5_RESP_FUNC PostSVM30Service
#define TIMER6_RESP_FUNC PostCloudService
#define TIMER7_RESP_FUNC TIMER_UNUSED
#define TIMER8_RESP_FUNC TIMER_UNUSED
#define TIMER9_RESP_FUNC TIMER_UNUSED
#define TIMER10_RESP_FUNC TIMER_UNUSED
#define TIMER11_RESP_FUNC TIMER_UNUSED
#define TIMER12_RESP_FUNC TIMER_UNUSED
#define TIMER13_RESP_FUNC TIMER_UNUSED
#define TIMER14_RESP_FUNC TIMER_UNUSED
#define TIMER15_RESP_FUNC PostTimerTestService

#define CO2_TIMER_NUM 0
#define HPM_TIMER_NUM 1
#define COMMSM_TIMER_NUM 2
#define MAIN_SERV_TIMER_NUM 3
#define SVM30_TIMER_NUM 4
#define SVM30_TIMER2_NUM 5
#define WIFI_TIMER_NUM 6


/********************************Services********************************************/
// The maximum number of services sets an upper bound on the number of
// services that the framework will handle. Reasonable values are 8 and 16
// corresponding to an 8-bit(uint8_t) and 16-bit(uint16_t) Ready variable size
#define MAX_NUM_SERVICES 16

/****************************************************************************/
// This macro determines that number of services that are *actually* used in
// a particular application. It will vary in value from 1 to MAX_NUM_SERVICES
#define NUM_SERVICES 8
#define QUEUE_SIZE 20  // min value should be NUM_SERVICES
// Note: queue sizes for individual services are not implemented yet. Currently there's just 1 queue, whose size
//       is defined by QUEUE_SIZE 

/****************************************************************************/
// These are the definitions for Service 0, the lowest priority service.
// Every Events and Services application must have a Service 0. Further
// services are added in numeric sequence (1,2,3,...) with increasing
// priorities
// the header file with the public function prototypes
#define SERV_0_HEADER "KeyboardService.h"
// the name of the Init function
#define SERV_0_INIT InitKeyboardService
// the name of the run function
#define SERV_0_RUN RunKeyboardService
// How big should this services Queue be?
// #define SERV_0_QUEUE_SIZE 5

/****************************************************************************/
// The following sections are used to define the parameters for each of the
// services. You only need to fill out as many as the number of services
// defined by NUM_SERVICES
/****************************************************************************/
// These are the definitions for Service 1
#if NUM_SERVICES > 1
// the header file with the public function prototypes
#define SERV_1_HEADER "ButtonService.h"
// the name of the Init function
#define SERV_1_INIT InitButtonService
// the name of the run function
#define SERV_1_RUN RunButtonService
// How big should this services Queue be?
// #define SERV_1_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 2
#if NUM_SERVICES > 2
// the header file with the public function prototypes
#define SERV_2_HEADER "TimerTestService.h"
// the name of the Init function
#define SERV_2_INIT InitTimerTestService
// the name of the run function
#define SERV_2_RUN RunTimerTestService
// How big should this services Queue be?
// #define SERV_2_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 3
#if NUM_SERVICES > 3
// the header file with the public function prototypes
#define SERV_3_HEADER "CO2_Service.h"
// the name of the Init function
#define SERV_3_INIT InitCO2Service
// the name of the run function
#define SERV_3_RUN RunCO2Service
// How big should this services Queue be?
// #define SERV_3_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 4
#if NUM_SERVICES > 4
// the header file with the public function prototypes
#define SERV_4_HEADER "HPM_Service.h"
// the name of the Init function
#define SERV_4_INIT InitHPMService
// the name of the run function
#define SERV_4_RUN RunHPMService
// How big should this services Queue be?
// #define SERV_4_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 5
#if NUM_SERVICES > 5
// the header file with the public function prototypes
#define SERV_5_HEADER "mainService.h"
// the name of the Init function
#define SERV_5_INIT InitMainService
// the name of the run function
#define SERV_5_RUN RunMainService
// How big should this services Queue be?
#define SERV_5_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 6
#if NUM_SERVICES > 6
// the header file with the public function prototypes
#define SERV_6_HEADER "SVM30Service.h"
// the name of the Init function
#define SERV_6_INIT InitSVM30Service
// the name of the run function
#define SERV_6_RUN RunSVM30Service
// How big should this services Queue be?
#define SERV_6_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 7
#if NUM_SERVICES > 7
// the header file with the public function prototypes
#define SERV_7_HEADER "CloudService.h"
// the name of the Init function
#define SERV_7_INIT InitCloudService
// the name of the run function
#define SERV_7_RUN RunCloudService
// How big should this services Queue be?
#define SERV_7_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 8
#if NUM_SERVICES > 8
// the header file with the public function prototypes
#define SERV_8_HEADER "TestHarnessService8.h"
// the name of the Init function
#define SERV_8_INIT InitTestHarnessService8
// the name of the run function
#define SERV_8_RUN RunTestHarnessService8
// How big should this services Queue be?
#define SERV_8_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 9
#if NUM_SERVICES > 9
// the header file with the public function prototypes
#define SERV_9_HEADER "TestHarnessService9.h"
// the name of the Init function
#define SERV_9_INIT InitTestHarnessService9
// the name of the run function
#define SERV_9_RUN RunTestHarnessService9
// How big should this services Queue be?
#define SERV_9_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 10
#if NUM_SERVICES > 10
// the header file with the public function prototypes
#define SERV_10_HEADER "TestHarnessService10.h"
// the name of the Init function
#define SERV_10_INIT InitTestHarnessService10
// the name of the run function
#define SERV_10_RUN RunTestHarnessService10
// How big should this services Queue be?
#define SERV_10_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 11
#if NUM_SERVICES > 11
// the header file with the public function prototypes
#define SERV_11_HEADER "TestHarnessService11.h"
// the name of the Init function
#define SERV_11_INIT InitTestHarnessService11
// the name of the run function
#define SERV_11_RUN RunTestHarnessService11
// How big should this services Queue be?
#define SERV_11_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 12
#if NUM_SERVICES > 12
// the header file with the public function prototypes
#define SERV_12_HEADER "TestHarnessService12.h"
// the name of the Init function
#define SERV_12_INIT InitTestHarnessService12
// the name of the run function
#define SERV_12_RUN RunTestHarnessService12
// How big should this services Queue be?
#define SERV_12_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 13
#if NUM_SERVICES > 13
// the header file with the public function prototypes
#define SERV_13_HEADER "TestHarnessService13.h"
// the name of the Init function
#define SERV_13_INIT InitTestHarnessService13
// the name of the run function
#define SERV_13_RUN RunTestHarnessService13
// How big should this services Queue be?
#define SERV_13_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 14
#if NUM_SERVICES > 14
// the header file with the public function prototypes
#define SERV_14_HEADER "TestHarnessService14.h"
// the name of the Init function
#define SERV_14_INIT InitTestHarnessService14
// the name of the run function
#define SERV_14_RUN RunTestHarnessService14
// How big should this services Queue be?
#define SERV_14_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 15
#if NUM_SERVICES > 15
// the header file with the public function prototypes
#define SERV_15_HEADER "TestHarnessService15.h"
// the name of the Init function
#define SERV_15_INIT InitTestHarnessService15
// the name of the run function
#define SERV_15_RUN RunTestHarnessService15
// How big should this services Queue be?
#define SERV_15_QUEUE_SIZE 3
#endif

#endif