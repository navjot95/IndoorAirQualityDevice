/****************************************************************************
 Module
   ePaperDriver.c

 Description
   Implements firmware for the higher level driver ePaper.

 Notes
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/

#include "ePaperDriver.h"
#include "ePaper.h"
#include "ES_Configure.h"
#include "UI_Display.h"
#include "IAQ_util.h"

/*----------------------------- Module Defines ----------------------------*/

#define TRH_Y_START 130
#define TRH_Y_LEN 120

#define SENSOR_Y_OFFSET 15
#define SENSOR_VAL_X_POS 31
#define SENSOR_LABEL_X_POS 68
#define SENSOR_VAL_TXT_LEN 42

#define BAT_EXTRA_LEN 6
#define MAX_BAT_LEVEL 100 
#define MAX_BAT_HEIGHT 18
#define BAT_X_OFFSET 3
#define BAT_Y_OFFSET 2
#define BAT_WIDTH 9

#define BAR_HEIGHT 65
#define BAR_WIDTH 10

#define HDLN_X_OFFSET (SCREEN_WIDTH - calibri_12ptFont.charHeight) 
#define HDLN2_Y_START 130


typedef struct _sensor
{
  const char *name; 
  const char *units; 
  uint16_t min_value;  // min sensor value for bar
  uint16_t max_value;  // max sensor value for bar
  uint16_t medThresh;  // cutoff for low/med/high label 
  uint16_t highThresh; // cutoff for low/med/high label 
  uint16_t yCord_start; // when screen is horizontal, this is where left side starts
  int16_t currAltVal;  // optional value to display additional reading  
  int16_t currValue; 
} sensor_t;


/*---------------------------- Module Functions ---------------------------*/
void setupHeader();
void displaySensor(sensor_t *thisSensor);
void updateBatLevel(uint8_t batLevel); 
void updateSensorVal(sensor_t *thisSensor, int16_t newVal, int16_t newAltVal);
uint8_t textVal(sensor_t *thisSensor, int16_t valToCalc);
void updateTempRH(int16_t temp, int16_t rh);


/*---------------------------- Module Variables ---------------------------*/
Epd epd = Epd(); 

static sensor_t eCO2 = 
  {
    .name = "eCO2", 
    .units = "ppm",
    .min_value = 0,
    .max_value = 2000, 
    .medThresh = 700, 
    .highThresh = 1000, 
    .yCord_start = 5,
    .currAltVal = -2,
    .currValue = -1
  };

static sensor_t tVOC = 
  {
    .name = "tVOC", 
    .units = "ppb",
    .min_value = 0,
    .max_value = 1000, 
    .medThresh = 100, 
    .highThresh = 250, 
    .yCord_start = 73,
    .currAltVal = -2,
    .currValue = -1
  };

static sensor_t pm25 = 
  {
    .name = "PM10/2.5", 
    .units = "ug/m3",
    .min_value = 0,
    .max_value = 250, 
    .medThresh = 25, 
    .highThresh = 50, 
    .yCord_start = 140,
    .currAltVal = -1,
    .currValue = -1 
  };

static sensor_t CO2 = 
  {
    .name = "CO2", 
    .units = "ppm",
    .min_value = 0,
    .max_value = 2000, 
    .medThresh = 710, 
    .highThresh = 1000, 
    .yCord_start = 235,
    .currAltVal = -2,
    .currValue = -1 
  };



/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     initePaper

 Parameters

 Returns
     bool, false if error in initialization, true otherwise

****************************************************************************/
bool initePaper()
{
  if (epd.Init() != 0) {
      printf("e-Paper init failed");
      return false;
  }

  // if(rtc_get_reset_reason(0) == PWR_RESET)
  // {
  epd.clearBuffer(); 
  displaySensor(&eCO2); 
  displaySensor(&tVOC); 
  displaySensor(&pm25); 
  displaySensor(&CO2);
  setupHeader();
  // }

  return true; 
}


void updateScreenSensorVals(IAQsensorVals_t *newVals, bool forceFullRefresh)
{
  updateSensorVal(&eCO2, newVals->eCO2, -2); 
  updateSensorVal(&tVOC, newVals->tVOC, -2); 
  updateSensorVal(&pm25, newVals->PM25, newVals->PM10);
  updateSensorVal(&CO2, newVals->CO2, -2); 
  updateTempRH(newVals->temp, newVals->rh); 
  epd.drawRect(HDLN_X_OFFSET - calibri_12ptFont.charHeight, HDLN2_Y_START, calibri_12ptFont.charHeight, 100, false);  // clear out any headline 2 text

  updateBatLevel(getBatPerct()); 
  updateEpaperTime(); 
  epd.updateScreen(forceFullRefresh); 
}

void ePaperPrintfAlert(const char * title, const char * line1, const char * line2)
{
  epd.drawRect(20,60,75,176,true); 
  epd.drawRect(22,62,71,172, false);
  epd.printf(title, &calibri_18ptFont, 70, 65); 
  epd.printf(line1, &calibri_12ptFont, 50, 65);
  epd.printf(line2, &calibri_12ptFont, 34, 65);
  updateBatLevel(getBatPerct()); 
  updateEpaperTime(); 
  epd.updateScreen(true); 
}

void ePaperChangeHdln(const char *txt, bool updateScrn)
{
  epd.drawRect(HDLN_X_OFFSET - calibri_12ptFont.charHeight, HDLN2_Y_START, calibri_12ptFont.charHeight, 100, false);  // clear out any prev text
  epd.printf(txt, &calibri_12ptFont, HDLN_X_OFFSET - calibri_12ptFont.charHeight,HDLN2_Y_START);
  updateBatLevel(getBatPerct()); 

  if(updateScrn)
    epd.updateScreen(false);
  
}

void ePaperChangeMode(IAQmode_t currMode)
{
  epd.drawRect(HDLN_X_OFFSET-calibri_10ptFont.charHeight, SCREEN_HEIGHT-40, calibri_10ptFont.charHeight, 40, false); 

  if(currMode == NO_MODE)
  {
    return; 
  }else if(currMode == STREAM_MODE)
  {
    epd.printf("Stream", &calibri_10ptFont, HDLN_X_OFFSET-calibri_10ptFont.charHeight, SCREEN_HEIGHT-40);
  }
  else
  {
    epd.printf("Auto", &calibri_10ptFont, HDLN_X_OFFSET-calibri_10ptFont.charHeight, SCREEN_HEIGHT-28);
  }
  
}


bool updateEpaperTime()
{
  bool returnVal; 
  time_t now;
  struct tm tmInfo;
  time(&now);
  localtime_r(&now, &tmInfo);

  uint8_t hr = (tmInfo.tm_hour % 12) ? (tmInfo.tm_hour % 12) : 12; 
  const char *dn = (tmInfo.tm_hour < 12) ? "AM" : "PM"; 
  char str[20]; 
  if(tmInfo.tm_year > (2019-1900))
  {
    returnVal = true; 
    sprintf(str, "%d/%d %d:%02d %s", tmInfo.tm_mon+1, tmInfo.tm_mday, hr, tmInfo.tm_min, dn);
  }
  else 
  {
    printf("Could not update time\n");
    returnVal = false; 
    sprintf(str, "00/00 00:00 AM");
  }

  epd.drawRect(HDLN_X_OFFSET - calibri_12ptFont.charHeight, 0, calibri_12ptFont.charHeight, 110, false); 
  epd.printf(str, &calibri_12ptFont, HDLN_X_OFFSET - calibri_12ptFont.charHeight,0);
  return returnVal; 
}


/***************************************************************************
 private functions
 ***************************************************************************/
void setupHeader()
{
  char headline1[] = "Updated:";
  char headline2[] = "00/00 00:00AM"; 
  epd.printf(headline1, &calibri_12ptFont, HDLN_X_OFFSET, 0);
  epd.printf(headline2, &calibri_12ptFont, HDLN_X_OFFSET - calibri_12ptFont.charHeight,0);

  epd.showImg(emptybatteryBitmaps, SCREEN_WIDTH-emptybatteryWidthPixels, SCREEN_HEIGHT-emptybatteryHeightPixels, emptybatteryWidthPixels, emptybatteryHeightPixels);  // draw battery body
  // epd.drawRect(SCREEN_WIDTH-emptybatteryWidthPixels+BAT_X_OFFSET, SCREEN_HEIGHT-emptybatteryHeightPixels+BAT_Y_OFFSET, BAT_WIDTH, 18, true); // make battery full
  updateBatLevel(getBatPerct()); 
} 


void displaySensor(sensor_t *thisSensor)
{
  epd.drawRect(0, thisSensor->yCord_start, BAR_HEIGHT, BAR_WIDTH, true); 
  epd.drawRect(1, thisSensor->yCord_start+1, BAR_HEIGHT-2, BAR_WIDTH-2, false);

  char valLabel[] = "Low"; 
  epd.printf(valLabel, &calibri_20ptBoldFont, SENSOR_LABEL_X_POS, thisSensor->yCord_start); 

  char sensorVal[] = "0000"; 
  epd.printf(sensorVal, &calibri_14ptFont, SENSOR_VAL_X_POS, thisSensor->yCord_start + SENSOR_Y_OFFSET); 
  if(thisSensor->currAltVal != -2)
  {
    epd.printf(sensorVal, &calibri_14ptFont, SENSOR_VAL_X_POS+calibri_14ptFont.charHeight, thisSensor->yCord_start + SENSOR_Y_OFFSET);
  }

  epd.printf(thisSensor->units, &calibri_10ptFont, 20, thisSensor->yCord_start + SENSOR_Y_OFFSET);
  epd.printf(thisSensor->name, &calibri_12ptFont, 0, thisSensor->yCord_start + SENSOR_Y_OFFSET); 

  if(strcmp(thisSensor->name, "PM10/2.5") == 0)
  {
    // manually adding labels for PM10 and PM2.5
    epd.printf("PM10", &calibri_10ptFont, 2+SENSOR_VAL_X_POS+calibri_14ptFont.charHeight, thisSensor->yCord_start + SENSOR_Y_OFFSET+SENSOR_VAL_TXT_LEN);
    epd.printf("PM2.5", &calibri_10ptFont, 2+SENSOR_VAL_X_POS, thisSensor->yCord_start + SENSOR_Y_OFFSET+SENSOR_VAL_TXT_LEN);
  }
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

void updateSensorVal(sensor_t *thisSensor, int16_t newVal, int16_t newAltVal)
{
  int16_t oldVal = thisSensor->currValue; 
  if(newVal == oldVal)
    return; 
  
  // redraw the digits and write "0000" in case sensor value is -1 
  char sensorVal[11];  // num of digits in 2^32 plus null char
  if(newVal < 0)
  {
    newVal = 0; 
    sprintf(sensorVal, "0000"); 
  }
  else
  {
    sprintf(sensorVal, "%i", newVal);  
  }
  epd.drawRect(SENSOR_VAL_X_POS, thisSensor->yCord_start + SENSOR_Y_OFFSET, calibri_14ptFont.charHeight, SENSOR_VAL_TXT_LEN, false);  // clear out previous text
  epd.printf(sensorVal, &calibri_14ptFont, SENSOR_VAL_X_POS, thisSensor->yCord_start + SENSOR_Y_OFFSET); 
  // redraw alt value if provided
  if(newAltVal != -2 && newAltVal != thisSensor->currAltVal)
  {
    if(newAltVal == -1)
      sprintf(sensorVal, "0000");
    else
      sprintf(sensorVal, "%i", newAltVal);
    epd.drawRect(SENSOR_VAL_X_POS+calibri_14ptFont.charHeight, thisSensor->yCord_start + SENSOR_Y_OFFSET, calibri_14ptFont.charHeight, SENSOR_VAL_TXT_LEN, false);  // clear out previous text
    epd.printf(sensorVal, &calibri_14ptFont, SENSOR_VAL_X_POS+calibri_14ptFont.charHeight, thisSensor->yCord_start + SENSOR_Y_OFFSET); 
    thisSensor->currAltVal = newAltVal; 
  }

  // set caps for the bar in case sensor val is higher than bar max value
  if(newVal > thisSensor->max_value)
    newVal = thisSensor->max_value; 
  else if(newVal < thisSensor->min_value)
    newVal = thisSensor->min_value; 

  //redraw the bar
  uint16_t bar_h = map(newVal, thisSensor->min_value, thisSensor->max_value, 1, BAR_HEIGHT-1); 
  uint16_t bar_h_old = map(oldVal, thisSensor->min_value, thisSensor->max_value, 1, BAR_HEIGHT-1);  
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

    epd.drawRect(SENSOR_LABEL_X_POS, thisSensor->yCord_start, calibri_20ptBoldFont.charHeight, 60, false);  // clear out previous text
    epd.printf(valLabel, &calibri_20ptBoldFont, SENSOR_LABEL_X_POS, thisSensor->yCord_start);
  }

  thisSensor->currValue = newVal; 
}

// returns values 1, 2, 3 depending on if display text for valToCal should be "Low", "Med", or "High"
uint8_t textVal(sensor_t *thisSensor, int16_t valToCalc)
{
  if(valToCalc < thisSensor->medThresh)
    return 1; 
  else if(valToCalc < thisSensor->highThresh)
    return 2; 
  else
    return 3; 
}

void updateTempRH(int16_t temp, int16_t rh)
{
  char strVal[20]; 
  if(temp < 0 || rh < 0)
    sprintf(strVal, "000F 000%% RH"); 
  else
    sprintf(strVal, "%iF %i%% RH", temp, rh); 
  
  epd.drawRect(SCREEN_WIDTH - calibri_12ptFont.charHeight, TRH_Y_START, calibri_12ptFont.charHeight, TRH_Y_LEN, false); 
  epd.printf(strVal, &calibri_12ptFont, SCREEN_WIDTH - calibri_12ptFont.charHeight, TRH_Y_START);
}
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

