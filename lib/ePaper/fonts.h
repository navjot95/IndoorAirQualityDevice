

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FONTS_H
#define __FONTS_H

#define MAX_HEIGHT_FONT         41
#define MAX_WIDTH_FONT          32
#define OFFSET_BITMAP           


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

typedef struct _charInfo
{
  uint16_t width; 
  uint16_t height;
  size_t offset; 
} charInfo_t; 

//ASCII
typedef struct _tFont
{
  uint16_t charHeight; 
  char startChar; 
  char endChar; 
  uint8_t spaceWidth; 
  const charInfo_t *charDescs; 
  const uint8_t *bitmap; 
} font_t;

// fonts generated through The Dot Factory app
// extern font_t calibri_22ptFont;  
extern font_t calibri_20ptBoldFont; 
extern font_t calibri_18ptFont; 
extern font_t calibri_14ptFont;
extern font_t calibri_12ptFont; 
extern font_t calibri_10ptFont; 

#endif /* __FONTS_H */
 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/