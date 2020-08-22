/****************************************************************************
 Module
   ePaperService.c

 Description
   Implements to firmware for the ePaper.

 Notes
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/

#include "ePaperService.h"
#include "ES_framework.h"
#include "ePaper.h"
#include "UI_Display.h"
#include <SPI.h>

/*----------------------------- Module Defines ----------------------------*/

#define TRH_Y_START 120
#define TRH_Y_LEN 120

#define SENSOR_Y_OFFSET 15
#define SENSOR_VAL_X_POS 35
#define SENSOR_LABEL_X_POS 70

#define BAT_EXTRA_LEN 6
#define MAX_BAT_LEVEL 100 
#define MAX_BAT_HEIGHT 18
#define BAT_X_OFFSET 3
#define BAT_Y_OFFSET 2
#define BAT_WIDTH 9

#define BAR_HEIGHT 65
#define BAR_WIDTH 10

typedef struct _sensor
{
  const char *name; 
  const char *units; 
  size_t min_value;  // min sensor value for bar
  size_t max_value;  // max sensor value for bar
  size_t medThresh;  // cutoff for low/med/high label 
  size_t highThresh; // cutoff for low/med/high label 
  size_t yCord_start; // when screen is horizontal, this is where left side starts
  size_t currValue; 
} sensor_t;


/*---------------------------- Module Functions ---------------------------*/
void setupHeader();
void displaySensor(sensor_t *thisSensor);
void updateBatLevel(uint8_t batLevel); 
void updateSensorVal(sensor_t *thisSensor, size_t newVal);
uint8_t textVal(sensor_t *thisSensor, size_t valToCalc);
void updateTempRH(uint8_t temp, uint8_t rh);


/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
Epd epd = Epd(false); 



/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitePaperService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and initializes the hw for the screen
 Notes
****************************************************************************/
bool InitePaperService(uint8_t Priority)
{
  ES_Event_t ThisEvent;
  MyPriority = Priority;
 

  if (epd.Init() != 0) {
      printf("e-Paper init failed");
      return false;
  }
  epd.clear(); 
  delay(2000); 

  sensor_t eCO2 = 
  {
    .name = "eCO2", 
    .units = "ppm",
    .min_value = 0,
    .max_value = 5000, 
    .medThresh = 1000, 
    .highThresh = 2000, 
    .yCord_start = 5,
    .currValue = 0
  };

  sensor_t tVOC = 
  {
    .name = "tVOC", 
    .units = "ppb",
    .min_value = 0,
    .max_value = 5000, 
    .medThresh = 1000, 
    .highThresh = 2000, 
    .yCord_start = 80,
    .currValue = 0
  };

  sensor_t pm = 
  {
    .name = "PM2.5", 
    .units = "AQI",
    .min_value = 0,
    .max_value = 500, 
    .medThresh = 50, 
    .highThresh = 100, 
    .yCord_start = 155,
    .currValue = 0 
  };

  sensor_t CO2 = 
  {
    .name = "CO2", 
    .units = "ppm",
    .min_value = 0,
    .max_value = 5000, 
    .medThresh = 1000, 
    .highThresh = 2000, 
    .yCord_start = 230,
    .currValue = 0 
  };


  displaySensor(&eCO2); 
  displaySensor(&tVOC); 
  displaySensor(&pm); 
  displaySensor(&CO2);
  setupHeader();  
  epd.updateScreen(); 


  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  ThisEvent.ServiceNum = Priority; 
  if (ES_PostToService(ThisEvent) == true)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/****************************************************************************
 Function
     PostePaperService

 Parameters
     EF_Event_t ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes
****************************************************************************/
bool PostePaperService(ES_Event_t ThisEvent)
{
  ThisEvent.ServiceNum = MyPriority; 
  return ES_PostToService(ThisEvent);
}

/****************************************************************************
 Function
    RunePaperService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Notes
****************************************************************************/
ES_Event_t RunePaperService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  /********************************************
   in here you write your service code
   *******************************************/
  return ReturnEvent;
}



/***************************************************************************
 private functions
 ***************************************************************************/

void setupHeader()
{
  char headline[] = "Sat Apr 15        77 F 53% RH       A"; 
  epd.printf(headline, &calibri_12ptFont, SCREEN_WIDTH - calibri_12ptFont.charHeight, 0);

  epd.showImg(emptybatteryBitmaps, SCREEN_WIDTH-emptybatteryWidthPixels, SCREEN_HEIGHT-emptybatteryHeightPixels, emptybatteryWidthPixels, emptybatteryHeightPixels);
  epd.drawRect(SCREEN_WIDTH-emptybatteryWidthPixels+BAT_X_OFFSET, SCREEN_HEIGHT-emptybatteryHeightPixels+BAT_Y_OFFSET, BAT_WIDTH, 18, true); // full battery
} 


void displaySensor(sensor_t *thisSensor)
{
  epd.drawRect(0, thisSensor->yCord_start, BAR_HEIGHT, BAR_WIDTH, true); 
  epd.drawRect(1, thisSensor->yCord_start+1, BAR_HEIGHT-2, BAR_WIDTH-2, false);

  char valLabel[] = "Low"; 
  epd.printf(valLabel, &calibri_20ptBoldFont, SENSOR_LABEL_X_POS, thisSensor->yCord_start); 

  char sensorVal[] = "0000"; 
  epd.printf(sensorVal, &calibri_14ptFont, SENSOR_VAL_X_POS, thisSensor->yCord_start + SENSOR_Y_OFFSET); 

  epd.printf(thisSensor->units, &calibri_10ptFont, 25, thisSensor->yCord_start + SENSOR_Y_OFFSET);
  epd.printf(thisSensor->name, &calibri_12ptFont, 0, thisSensor->yCord_start + SENSOR_Y_OFFSET); 

  return; 
}

// provide value between 0-100 
void updateBatLevel(uint8_t batLevel)
{
  if(batLevel > MAX_BAT_LEVEL)
    batLevel = MAX_BAT_LEVEL; 

  uint8_t yLen = (batLevel * (emptybatteryHeightPixels - BAT_EXTRA_LEN)) / MAX_BAT_LEVEL;
  epd.drawRect(SCREEN_WIDTH-emptybatteryWidthPixels+BAT_X_OFFSET, SCREEN_HEIGHT-emptybatteryHeightPixels+BAT_Y_OFFSET, BAT_WIDTH, yLen, true); 
  epd.drawRect(SCREEN_WIDTH-emptybatteryWidthPixels+BAT_X_OFFSET, SCREEN_HEIGHT-emptybatteryHeightPixels+BAT_Y_OFFSET+yLen, BAT_WIDTH, MAX_BAT_HEIGHT-yLen, false); 
}

void updateSensorVal(sensor_t *thisSensor, size_t newVal)
{
  size_t oldVal = thisSensor->currValue; 
  if(newVal == oldVal)
    return; 
  
  // redraw the bar 
  size_t bar_h = map(newVal, thisSensor->min_value, thisSensor->max_value, 1, BAR_HEIGHT-1); 
  size_t bar_h_old = map(oldVal, thisSensor->min_value, thisSensor->max_value, 1, BAR_HEIGHT-1);  
  if(newVal > oldVal)
  {
    epd.drawRect(bar_h_old, thisSensor->yCord_start+1, bar_h-bar_h_old, BAR_WIDTH-2, true); 
  }else
  {
    epd.drawRect(bar_h, thisSensor->yCord_start+1, BAR_HEIGHT-bar_h-1, BAR_WIDTH-2, false);
  }

  // redraw the category label, if needed (but more often val will stay in the same category)
  uint8_t newTextVal = textVal(thisSensor, newVal);
  if(textVal(thisSensor, oldVal) != newTextVal)
  {
    const char *valLabel; 
    if(newTextVal == 1)
      valLabel = "Low"; 
    else if(newTextVal == 2)
      valLabel = "Med";
    else 
      valLabel = "High"; 
      
    epd.drawRect(SENSOR_LABEL_X_POS, thisSensor->yCord_start, calibri_20ptBoldFont.charHeight, 40, false);  // clear out previous text
    epd.printf(valLabel, &calibri_20ptBoldFont, SENSOR_LABEL_X_POS, thisSensor->yCord_start);
  }

  // redraw the value digits 
  char sensorVal[11];  // num of digits in 2^32 plus null char
  sprintf(sensorVal, "%i", newVal);  
  epd.drawRect(SENSOR_VAL_X_POS, thisSensor->yCord_start + SENSOR_Y_OFFSET, calibri_14ptFont.charHeight, 50, false);  // clear out previous text
  epd.printf(sensorVal, &calibri_14ptFont, SENSOR_VAL_X_POS, thisSensor->yCord_start + SENSOR_Y_OFFSET); 

  thisSensor->currValue = newVal; 
}

// returns values 0, 1, 2 depending of if display text for valToCal should be "Low", "Med", or "High"
uint8_t textVal(sensor_t *thisSensor, size_t valToCalc)
{
  if(valToCalc < thisSensor->medThresh)
    return 1; 
  else if(valToCalc < thisSensor->highThresh)
    return 2; 
  else
    return 3; 
}

void updateTempRH(uint8_t temp, uint8_t rh)
{
  char strVal[20]; 
  sprintf(strVal, "%i F %i%% RH", temp, rh); 
  epd.drawRect(SCREEN_WIDTH - calibri_12ptFont.charHeight, TRH_Y_START, calibri_12ptFont.charHeight, TRH_Y_LEN, false); 
  epd.printf(strVal, &calibri_12ptFont, SCREEN_WIDTH - calibri_12ptFont.charHeight, TRH_Y_START);
}
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

