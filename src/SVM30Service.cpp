/****************************************************************************
 Module
   SVM30Service.c

 Description
   Service the reads tVOC, eCO2, temperature, and relative humidity values 
   from the SVM30 sensor. 

 Notes
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
#include "SVM30Service.h"
#include "ES_framework.h"
#include "ES_Timers.h"
#include "Wire.h"
#include <stdio.h>

/*----------------------------- Module Defines ----------------------------*/
// #define DEBUG_SVM30

#define SVM30_STARTUP_TIME 20
#define SVM30_RESP_LEN 6
#define SVM30_WAIT_TIME 20

// eCO2 and tVOC sensor
#define SGP30_FLAG true
#define SGP30_ADDR 0x58
#define SGP30_HEAD 0x20
#define SGP30_INIT_AQ 0x03
#define SGP30_MEASURE_AQ 0X08
#define SGP30_WARMUP_TIME 15000
#define SGP30_SAMPLE_TIME 1000
// temp and RH sensor
#define SHTC1_FLAG false 
#define SHTC1_ADDR 0X70
#define TEMP_FIRST1 0x78
#define TEMP_FIRST2 0X66
#define RH_FIRST1 0X58
#define RH_FIRST2 0xE0 

#define MAX_RETRY_ATTEMPTS 2
#define SVM30_SAMPLE_READS 16  // total samples to read, including NUM_SAMPLES_SKIP
#define NUM_SAMPLES_SKIP 8  // Let's sensor's baseline algo settle before recording
#define I2C_SUCCESS 0

typedef enum{
  INIT_MEASUREMENTS_STATE,
  WARMUP_STATE,
  REQUEST_READ_STATE,
  READ_DATA_STATE
} SMStatusState_t; 

/*---------------------------- Module Functions ---------------------------*/
void check_sum(); 
void printTempRHValues(); 
void printAirQualityValues(); 
uint8_t crcCheck(uint8_t* p, int len);
void clearI2CBuffer();
bool retryRead(uint8_t *retryAttempts, uint8_t *bytesRead, uint8_t *data_idx, bool *sensorFlag);
float rawDataToRH(uint16_t temp_raw, uint16_t rh_raw);
float rawDataToTemp(uint16_t temp_raw);

/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
static SMStatusState_t currSMState = INIT_MEASUREMENTS_STATE; 

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitSVM30Service

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes
****************************************************************************/
bool InitSVM30Service(uint8_t Priority)
{
  MyPriority = Priority;

  return true; 
}

/****************************************************************************
 Function
     PostSVM30Service

 Parameters
     EF_Event_t ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes
****************************************************************************/
bool PostSVM30Service(ES_Event_t ThisEvent)
{
  ThisEvent.ServiceNum = MyPriority; 
  return ES_PostToService(ThisEvent);
}

/****************************************************************************
 Function
    RunSVM30Service

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
****************************************************************************/
ES_Event_t RunSVM30Service(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  static uint8_t retryAttempts = 0; 
  static uint8_t numReads = 0;  // num of sensor readings completed for the avg
  static bool SGP30_SHTC1_flag = SGP30_FLAG; 
  
  switch (currSMState)
  {
    case INIT_MEASUREMENTS_STATE:
    {
      if(ThisEvent.EventType == ES_READ_SENSOR)
      {
        ES_Timer_InitTimer(SVM30_TIMER_NUM, SVM30_STARTUP_TIME); // wait for sensor to power up 
      }
      else if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == SVM30_TIMER_NUM)
      {
        clearI2CBuffer();  // clear out buffer just to be safe
        Wire.begin();
        Wire.beginTransmission(SGP30_ADDR); 
        Wire.write(SGP30_HEAD);
        Wire.write(SGP30_INIT_AQ); 
        uint8_t I2C_resp = Wire.endTransmission();
        if(I2C_resp != I2C_SUCCESS) 
        {
          #ifdef DEBUG_SVM30
          printf("Init measurement command failed\n");
          #endif
          retryRead(&retryAttempts, NULL, NULL, &SGP30_SHTC1_flag); 
          return ReturnEvent; 
        }

        #ifdef DEBUG_SVM30
        printf("Starting SGP30 15sec wait\n");
        #endif
        ES_Timer_InitTimer(SVM30_TIMER_NUM, SGP30_WARMUP_TIME);
        currSMState = WARMUP_STATE; 
      }
      break;
    }
    case WARMUP_STATE:
    {
      if(ThisEvent.EventType == ES_READ_SENSOR || (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == SVM30_TIMER_NUM))
      {
        uint8_t I2C_resp; 
        clearI2CBuffer();
        if(SGP30_SHTC1_flag == SGP30_FLAG)
        {
          ES_Timer_InitTimer(SVM30_TIMER_NUM, SGP30_SAMPLE_TIME);
          Wire.beginTransmission(SGP30_ADDR);  
          Wire.write(SGP30_HEAD);
          Wire.write(SGP30_MEASURE_AQ); 
          I2C_resp = Wire.endTransmission();
        }
        else
        {
          Wire.beginTransmission(SHTC1_ADDR); 
          Wire.write(TEMP_FIRST1);
          Wire.write(TEMP_FIRST2); 
          I2C_resp = Wire.endTransmission();
        }
        
        if(I2C_resp != I2C_SUCCESS) 
        {
          #ifdef DEBUG_SVM30
          printf("Measure command failed\n");
          #endif

          retryRead(&retryAttempts, NULL, NULL, &SGP30_SHTC1_flag); 
          return ReturnEvent; 
        }

        ES_Timer_InitTimer(SVM30_TIMER2_NUM, SVM30_WAIT_TIME);
        currSMState = REQUEST_READ_STATE;
      }
      break;
    }

    case REQUEST_READ_STATE:  // waiting to send request data after sending init measurement command
    {
      if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == SVM30_TIMER2_NUM)
      {
        if(SGP30_SHTC1_flag == SGP30_FLAG)
          Wire.requestFrom(SGP30_ADDR, SVM30_RESP_LEN);
        else
          Wire.requestFrom(SHTC1_ADDR, SVM30_RESP_LEN);
        
        currSMState = READ_DATA_STATE; 
      }
      else if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == SVM30_TIMER_NUM)
      {
        retryRead(&retryAttempts, NULL, NULL, &SGP30_SHTC1_flag); 
        return ReturnEvent; 
      }
      break;
    } 

    case READ_DATA_STATE:
    {
      static uint8_t bytesread = 1;  // bytes read in a single transmission
      static uint8_t data_idx = 0; 
      static float eCO2_avg, tVOC_avg, tm_avg, rh_avg = 0; 
      static uint8_t sensorData[4] = {0};  

      if(ThisEvent.EventType == ES_I2C)
      {
        // #ifdef DEBUG_SVM30
        // printf("I2C bytes read (%d): %d\n", bytesread, ThisEvent.EventParam); 
        // #endif

        if(numReads < NUM_SAMPLES_SKIP)  // skipping first few reads to let sensor algo settle 
        {
          #ifdef DEBUG_SVM30
          printf("%d: Skipping\n", numReads); 
          #endif

          if(bytesread == SVM30_RESP_LEN)
          {
            bytesread = 1; 
            data_idx = 0; 
            numReads++; 
            currSMState = WARMUP_STATE; 
          }
          else
          {
            bytesread++; 
          }
          return ReturnEvent; 
        }

        if(bytesread % 3 == 0)  // is it a CRC byte?
        { 
          uint8_t *startIdx = sensorData + (data_idx-2); 
          uint8_t crcCheckVal = crcCheck(startIdx, 2);
          if(crcCheckVal != ThisEvent.EventParam){
            retryRead(&retryAttempts, &bytesread, &data_idx, &SGP30_SHTC1_flag);
            return ReturnEvent;  
          }
        }
        else  // otherwise it's a data byte
        {
          sensorData[data_idx] = ThisEvent.EventParam; 
          data_idx++; 
        }

        if(bytesread == SVM30_RESP_LEN)
        {          
          uint16_t dataByte1_raw = (uint16_t)sensorData[0] << 8; 
          dataByte1_raw |= (uint16_t)sensorData[1]; 

          uint16_t dataByte2_raw = (uint16_t)sensorData[2] << 8; 
          dataByte2_raw |= (uint16_t)sensorData[3]; 
          
          // reset for the next reading
          bytesread = 1; 
          data_idx = 0; 
          currSMState = WARMUP_STATE; 
          if(SGP30_SHTC1_flag == SGP30_FLAG)  
          {
            #ifdef DEBUG_SVM30
            printf("eCO2: %i  ", dataByte1_raw); 
            printf("tvoc: %i\n", dataByte2_raw); 
            #endif

            eCO2_avg += dataByte1_raw;  
            tVOC_avg += dataByte2_raw;
            SGP30_SHTC1_flag = SHTC1_FLAG;  // switch to the other sensor 
            ES_Event_t NewEvent = {.EventType=ES_READ_SENSOR}; 
            PostSVM30Service(NewEvent); 
          }
          else 
          {
            //now finished reading 2nd sensor
            SGP30_SHTC1_flag = SGP30_FLAG; 
            tm_avg += rawDataToTemp(dataByte1_raw);
            rh_avg += rawDataToRH(dataByte1_raw, dataByte2_raw);

            numReads++; 
            if(numReads == SVM30_SAMPLE_READS)
            {
              eCO2_avg /= (SVM30_SAMPLE_READS - NUM_SAMPLES_SKIP);
              tVOC_avg /= (SVM30_SAMPLE_READS - NUM_SAMPLES_SKIP);
              tm_avg /= (SVM30_SAMPLE_READS - NUM_SAMPLES_SKIP);
              rh_avg /= (SVM30_SAMPLE_READS - NUM_SAMPLES_SKIP);
              updateSVM30Vals(round(eCO2_avg), round(tVOC_avg), round(tm_avg), round(rh_avg)); 
              printf("Avgs:  eCO2:%.2f  tVOC:%.2f  tm:%.02f  rh:%.02f\n", eCO2_avg, tVOC_avg, tm_avg, rh_avg); 

              retryAttempts = 0; 
              numReads = 0; 
              eCO2_avg = 0; 
              tVOC_avg = 0; 
              tm_avg = 0; 
              rh_avg = 0; 
              currSMState = INIT_MEASUREMENTS_STATE; 
              ES_Timer_StopTimer(SVM30_TIMER_NUM);
            }
          }
        }else
        {
          bytesread++;
        }
      }
      else if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == SVM30_TIMER_NUM)
      {
        #ifdef DEBUG_SVM30
        printf("Timed out in READ STATE\n");
        #endif 
        retryRead(&retryAttempts, &bytesread, &data_idx, &SGP30_SHTC1_flag); 
      }

      break;
    }
  }
  
  return ReturnEvent;
}


bool EventChecker_SVM30(){
  if(Wire.available())
  {
    ES_Event_t newEvent; 
    newEvent.EventType = ES_I2C; 
    newEvent.EventParam = Wire.read(); 
    PostSVM30Service(newEvent); 

    return true; 
  }
  return false; 
}


/***************************************************************************
 private functions
 ***************************************************************************/
bool retryRead(uint8_t *retryAttempts, uint8_t *bytesRead, uint8_t *data_idx, bool *sensorFlag)
{
  if(retryAttempts == NULL)
    return false; 
  if(bytesRead != NULL)
    *bytesRead = 1; 
  if(data_idx != NULL)
    *data_idx = 0; 

  bool skipWarmup = false; 
  if(*sensorFlag == SHTC1_FLAG)
    skipWarmup = true;

  if(skipWarmup)
    currSMState = WARMUP_STATE; 
  else
    currSMState = INIT_MEASUREMENTS_STATE; 
  

  if(*retryAttempts >= MAX_RETRY_ATTEMPTS)
  {
    currSMState = INIT_MEASUREMENTS_STATE; 
    *retryAttempts = 0; 
    ES_Timer_StopTimer(SVM30_TIMER_NUM);
    ES_Timer_StopTimer(SVM30_TIMER2_NUM); 
    *sensorFlag = SGP30_FLAG; 
    updateSVM30Vals(-1, -1, -1, -1); 
    printf("Could not read from SVM30 sensor\n");
    return false; 
  }

  printf("Retrying SVM30 sensor: %d\n", *sensorFlag);
  ES_Timer_InitTimer(SVM30_TIMER_NUM, 200);
  *retryAttempts = *retryAttempts + 1; 
  return true; 
}

float rawDataToTemp(uint16_t temp_raw)
{
  float temp = -45.68 + 175.7 * ((float)temp_raw/((float)UINT16_MAX+1.0));
  temp = (temp * (9.0/5.0)) + 32; 
  #ifdef DEBUG_SVM30
  printf("Temp: %.2fF  ", temp); 
  #endif
  return temp; 
}

float rawDataToRH(uint16_t temp_raw, uint16_t rh_raw)
{
  float rh = (103.7 - 3.2*((float)temp_raw/((float)UINT16_MAX+1.0))) * ((float)rh_raw/((float)UINT16_MAX+1.0));
  #ifdef DEBUG_SVM30
  printf("RH: %.2f\n", rh);
  #endif
  return rh; 
}

uint8_t crcCheck(uint8_t* p, int len) {
	// fast bit by bit algorithm without augmented zero bytes
  // taken from http://www.zorc.breitbandkatze.de/crctester.c
    
  const unsigned int order = 8;
  const unsigned int polynom = 0x31;
  const unsigned int crcinit = 0xFF;
  const unsigned int crcxor = 0x00;

  unsigned int crchighbit = 1<<(order-1);
  unsigned int crcmask = (((1<<(order-1))-1)<<1)|1;

	unsigned int i, j, c, bit;
	unsigned int crc = crcinit;

	for (i=0; i<len; i++) {
		c = *p++;

		for (j=0x80; j; j>>=1) {
			bit = crc & crchighbit;
			crc <<= 1;
			if(c & j) bit ^= crchighbit;
			if(bit) crc ^= polynom;
		}
	}	

	crc ^= crcxor;
	crc &= crcmask;

	return(crc);
}

void clearI2CBuffer()
{
  while(Wire.available())
  {
    printf("Cleared data CO2\n");
    Wire.read(); 
  }
}
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

