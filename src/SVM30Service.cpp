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
#define SGP30_WARMUP_TIME 16000
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
uint16_t rawDataToRH(uint16_t temp_raw, uint16_t rh_raw);
uint16_t rawDataToTemp(uint16_t temp_raw);

/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
static SMStatusState_t currSMState = INIT_MEASUREMENTS_STATE; 
static IAQmode_t SVM30_mode = STREAM_MODE; 
static uint8_t numReads = 0;  // num of sensor readings completed for the avg
static bool sensorConnected = false; 
static runAvg_t eCO2RunAvg = {.runAvgSum=0, .buff={0}, .oldestIdx=0};
static runAvg_t tVOCRunAvg = {.runAvgSum=0, .buff={0}, .oldestIdx=0};
static runAvg_t tempRunAvg = {.runAvgSum=0, .buff={0}, .oldestIdx=0};
static runAvg_t rhRunAvg = {.runAvgSum=0, .buff={0}, .oldestIdx=0};

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
  Wire.begin();
  memset(eCO2RunAvg.buff, 0, sizeof(eCO2RunAvg.buff));
  memset(tVOCRunAvg.buff, 0, sizeof(tVOCRunAvg.buff));
  memset(tempRunAvg.buff, 0, sizeof(tempRunAvg.buff));
  memset(rhRunAvg.buff, 0, sizeof(rhRunAvg.buff));

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
      static uint8_t sensorData[4] = {0};  

      if(ThisEvent.EventType == ES_I2C)
      {
        // #ifdef DEBUG_SVM30
        // printf("I2C bytes read (%d): %d\n", bytesread, ThisEvent.EventParam); 
        // #endif

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

            updateRunAvg(&eCO2RunAvg, dataByte1_raw);  
            updateRunAvg(&tVOCRunAvg, dataByte2_raw);
            SGP30_SHTC1_flag = SHTC1_FLAG;  // switch to the other sensor 
            ES_Event_t NewEvent = {.EventType=ES_READ_SENSOR}; 
            PostSVM30Service(NewEvent); 
          }
          else 
          {
            sensorConnected = true; 
            //now finished reading 2nd sensor
            SGP30_SHTC1_flag = SGP30_FLAG; 
            updateRunAvg(&tempRunAvg, rawDataToTemp(dataByte1_raw));
            updateRunAvg(&rhRunAvg, rawDataToRH(dataByte1_raw, dataByte2_raw));
            // tm_avg += rawDataToTemp(dataByte1_raw);
            // rh_avg += rawDataToRH(dataByte1_raw, dataByte2_raw);

            if(SVM30_mode == STREAM_MODE)
              numReads = 0; 
            else
              numReads++; 

            if(numReads >= SVM30_SAMPLE_READS)
            {
              int16_t avgeCO2 =  round((float)eCO2RunAvg.runAvgSum / (float)RUN_AVG_BUFFER_LEN);
              int16_t avgtVOC  = round((float)tVOCRunAvg.runAvgSum / (float)RUN_AVG_BUFFER_LEN); 
              int16_t avgtm =  round((float)tempRunAvg.runAvgSum / (float)RUN_AVG_BUFFER_LEN);
              int16_t avgrh  = round((float)rhRunAvg.runAvgSum / (float)RUN_AVG_BUFFER_LEN); 

              updateSVM30Vals(avgeCO2, avgtVOC, avgtm, avgrh); 
              IAQ_PRINTF("Avgs:  eCO2:%d  tVOC:%d  tm:%d  rh:%d\n", avgeCO2, avgtVOC, avgtm, avgrh); 

              retryAttempts = 0; 
              numReads = 0; 
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


void setModeSVM30(IAQmode_t newMode)
{
  SVM30_mode = newMode; 
}


void getSVM30Avg(int16_t *eCO2Avg, int16_t *tVOCAvg, int16_t *tpAvg, int16_t *rhAvg)
{
  if(sensorConnected)
  {
    *eCO2Avg =  round((float)eCO2RunAvg.runAvgSum / (float)RUN_AVG_BUFFER_LEN);
    *tVOCAvg  = round((float)tVOCRunAvg.runAvgSum / (float)RUN_AVG_BUFFER_LEN); 
    *tpAvg =  round((float)tempRunAvg.runAvgSum / (float)RUN_AVG_BUFFER_LEN);
    *rhAvg  = round((float)rhRunAvg.runAvgSum / (float)RUN_AVG_BUFFER_LEN); 
  }else
  {
    *eCO2Avg = -1; 
    *tVOCAvg = -1; 
    *tpAvg = -1; 
    *rhAvg = -1; 
  }

  IAQ_PRINTF("eCO2 run avg: %d    tVOC run avg: %d     tp run avg: %d     rh run avg: %d\n", *eCO2Avg, *tVOCAvg, *tpAvg, *rhAvg); 
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

  sensorConnected = false; 

  bool skipWarmup = false; 
  if(*sensorFlag == SHTC1_FLAG)
    skipWarmup = true;

  if(skipWarmup)
    currSMState = WARMUP_STATE; 
  else
    currSMState = INIT_MEASUREMENTS_STATE; 
  
  if(SVM30_mode == STREAM_MODE)
  {
    *retryAttempts = 0;  // in stream mode don't need to stop trying
  }

  if(*retryAttempts >= MAX_RETRY_ATTEMPTS)
  {
    currSMState = INIT_MEASUREMENTS_STATE; 
    numReads = 0; 
    *retryAttempts = 0; 
    ES_Timer_StopTimer(SVM30_TIMER_NUM);
    ES_Timer_StopTimer(SVM30_TIMER2_NUM); 
    *sensorFlag = SGP30_FLAG; 
    updateSVM30Vals(-1, -1, -1, -1); 
    IAQ_PRINTF("Could not read from SVM30 sensor\n");
    return false; 
  }

  IAQ_PRINTF("Retrying SVM30 sensor: %d\n", *sensorFlag);
  ES_Timer_InitTimer(SVM30_TIMER_NUM, 200);
  *retryAttempts = *retryAttempts + 1; 
  return true; 
}

uint16_t rawDataToTemp(uint16_t temp_raw)
{
  float temp = -45.68 + 175.7 * ((float)temp_raw/((float)UINT16_MAX+1.0));
  temp = (temp * (9.0/5.0)) + 32; 
  #ifdef DEBUG_SVM30
  printf("Temp: %.2fF  ", temp); 
  #endif
  return (uint16_t)round(temp); 
}

uint16_t rawDataToRH(uint16_t temp_raw, uint16_t rh_raw)
{
  float rh = (103.7 - 3.2*((float)temp_raw/((float)UINT16_MAX+1.0))) * ((float)rh_raw/((float)UINT16_MAX+1.0));
  #ifdef DEBUG_SVM30
  printf("RH: %.2f\n", rh);
  #endif
  return (uint16_t)round(rh); 
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
    IAQ_PRINTF("Cleared data CO2\n");
    Wire.read(); 
  }
}
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

