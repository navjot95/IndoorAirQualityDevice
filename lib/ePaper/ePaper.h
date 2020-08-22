/**
 *  @filename   :   ePaper.h
 *  @brief      :   Header file for e-paper display library
 *  @author     :   Navjot Singh
 *  
 * 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef EPD2IN9_H
#define EPD2IN9_H

// #define DEBUG 

#include "epdif.h"
#include "fonts.h"


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 296
#define BUFFER_SIZE (SCREEN_WIDTH*SCREEN_HEIGHT)/8



class Epd : EpdIf {
public:
    Epd(bool partialRefreshMode);
    ~Epd();
    int  Init(void);
    void Sleep(void);
    void clear(void);
    int8_t printf(const char *strToPrint, font_t *scrnFont, size_t xPos, size_t yPos);  
    void updateScreen(); 
    int8_t showImg(const uint8_t *img, size_t topLeft_x, size_t topLeft_y, size_t xLen, size_t yLen); 
    int8_t drawRect(size_t topLeft_x, size_t topLeft_y, size_t xLen, size_t yLen, bool black);
    void ChangeRefreshMode(bool partial); 

private:
    unsigned int reset_pin;
    unsigned int dc_pin;
    unsigned int cs_pin;
    unsigned int busy_pin;
    unsigned long width;
    unsigned long height;
    bool partialRefresh; 
    uint8_t frame_buffer_black[BUFFER_SIZE]; 

    void DisplayFrame(const uint8_t* img);
    void SendCommand(unsigned char command);
    void SendData(unsigned char data);
    void WaitUntilIdle(void);
    void Reset(void);
    void SetWindow(size_t Xstart, size_t Ystart, size_t Xend, size_t Yend); 
    void SetCursor(size_t Xstart, size_t Ystart); 
    void TurnOnDisplay(void);
    size_t getAsciiTableIdx(const char val, font_t *scrnFont);
    size_t getBufferIdx(size_t xCursor, size_t yCursor);
    void copyCharToBuffer(char charToPrint, size_t xCursor, size_t yCursor, font_t *scrnFont);
    void copyLineToBuffer(size_t bitPos, size_t xBitLen, const uint8_t *lineData);
    uint16_t calcXByteLength(size_t xBitLen); 
    uint16_t getCharWidth(char thisChar, font_t *scrnFont);
    int8_t copyImgToBuffer(size_t tableByteIdx, size_t topLeft_x, size_t topLeft_y, const uint8_t *img, size_t xLen, size_t yLen);
};

#endif 

/* END OF FILE */
