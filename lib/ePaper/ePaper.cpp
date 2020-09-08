/**
 *  @filename   :   ePaper.cpp
 *  @brief      :   Implements for e-paper library
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

#include <stdlib.h>
#include "ePaper.h"


#define PARTIAL_REFRESH_LIMIT 4  // full refresh every this many refreshes, otherwise partial refresh
RTC_DATA_ATTR static uint8_t refreshCounter = 1; 

static const uint8_t lut_full_update[] = {
    0x50, 0xAA, 0x55, 0xAA, 0x11, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0x1F, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t lut_partial_update[] = {
    0x10, 0x18, 0x18, 0x08, 0x18, 0x18,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x13, 0x14, 0x44, 0x12,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


Epd::~Epd() {
};

Epd::Epd() {
    reset_pin = RST_PIN;
    dc_pin = DC_PIN;
    cs_pin = CS_PIN;
    busy_pin = BUSY_PIN;
    width = SCREEN_WIDTH;
    height = SCREEN_HEIGHT;

    memset(frame_buffer_black, 0X00, sizeof(frame_buffer_black));
};

int Epd::Init(void) {
    /* this calls the peripheral hardware interface, see epdif */
    if (IfInit() != 0) {
        return -1;
    }
    /* EPD hardware init start */
    Reset();

    SendCommand(0x01); // DRIVER_OUTPUT_CONTROL
    SendData((SCREEN_HEIGHT - 1) & 0xFF);
    SendData(((SCREEN_HEIGHT - 1) >> 8) & 0xFF);
    SendData(0x00); // GD = 0; SM = 0; TB = 0;
	
    SendCommand(0x0C); // BOOSTER_SOFT_START_CONTROL
    SendData(0xD7);
    SendData(0xD6);
    SendData(0x9D);
	
    SendCommand(0x2C); // WRITE_VCOM_REGISTER
    SendData(0xA8); // VCOM 7C
	
    SendCommand(0x3A); // SET_DUMMY_LINE_PERIOD
    SendData(0x1A); // 4 dummy lines per gate
	
    SendCommand(0x3B); // SET_GATE_TIME
    SendData(0x08); // 2us per line
	
    SendCommand(0x3C); // BORDER_WAVEFORM_CONTROL
    SendData(0x03);                     
    SendCommand(0x11); // DATA_ENTRY_MODE_SETTING
    SendData(0x03);

    /* EPD hardware init end */

    return 0;
}

void Epd::ChangeRefreshMode(bool partialRefresh)
{
    //set the look-up table register
    SendCommand(0x32); // WRITE_LUT_REGISTER
    const unsigned char *lut; 
    if(partialRefresh)
    {
        lut = lut_partial_update;  
    }
    else {
        lut = lut_full_update;
    }

    for (size_t i = 0; i < 30; i++) {
        SendData(lut[i]);
    }
    
}

/**
 *  @brief: basic function for sending commands
 */
void Epd::SendCommand(unsigned char command) {
    DigitalWrite(dc_pin, LOW);
    SpiTransfer(command);
}

/**
 *  @brief: basic function for sending data
 */
void Epd::SendData(unsigned char data) {
    DigitalWrite(dc_pin, HIGH);
    SpiTransfer(data);
}

/**
 *  @brief: Wait until the busy_pin goes HIGH
 */
void Epd::WaitUntilIdle(void) {
    while(DigitalRead(busy_pin) == 0) {      //0: busy, 1: idle
        DelayMs(50);
    }      
}

/**
 *  @brief: module reset.
 *          used to awaken the module in deep sleep,
 *          see Epd::Sleep();
 */
void Epd::Reset(void) {
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);  
    DigitalWrite(reset_pin, LOW);                //module reset    
    DelayMs(200);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);    
}



void Epd::SetWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend)
{
    // this is from sample code
    SendCommand(0x44); // SET_RAM_X_ADDRESS_START_END_POSITION
    SendData((Xstart >> 3) & 0xFF);
    SendData((Xend >> 3) & 0xFF);

    SendCommand(0x45); // SET_RAM_Y_ADDRESS_START_END_POSITION
    SendData(Ystart & 0xFF);
    SendData((Ystart >> 8) & 0xFF);
    SendData(Yend & 0xFF);
    SendData((Yend >> 8) & 0xFF);
}


void Epd::SetCursor(uint16_t Xstart, uint16_t Ystart)
{
    SendCommand(0x4E); // SET_RAM_X_ADDRESS_COUNTER
    SendData((Xstart >> 3) & 0xFF);

    SendCommand(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
    SendData(Ystart & 0xFF);
    SendData((Ystart >> 8) & 0xFF);
}

void Epd::TurnOnDisplay(void)
{
    SendCommand(0x22); // DISPLAY_UPDATE_CONTROL_2
    SendData(0xC4);
    SendCommand(0x20); // MASTER_ACTIVATION
    SendCommand(0xFF); // TERMINATE_FRAME_READ_WRITE

    WaitUntilIdle();
}



void Epd::DisplayFrame(const uint8_t* img) {
    size_t byteWidth = width / 8;
    size_t addr_idx = 0; 

    SetWindow(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    for (size_t j = 0; j < SCREEN_HEIGHT; j++) {
        SetCursor(0, j);
        SendCommand(0x24);
        for (size_t i = 0; i < byteWidth; i++) {
            addr_idx = i + (j * byteWidth);
            SendData(~img[addr_idx]);
        }
    }
    TurnOnDisplay();
    Sleep(); 
}

/**
 *  @brief: After this command is transmitted the screen 
 *  is turned white 
 */
void Epd::clearePaper(void){
    size_t byteWidth = SCREEN_WIDTH / 8;

    SetWindow(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    for (size_t j = 0; j < SCREEN_HEIGHT; j++) {
        SetCursor(0, j);
        SendCommand(0x24);
        for (size_t i = 0; i < byteWidth; i++) {
            SendData(0XFF);
        }
    }
    TurnOnDisplay();
}

/**
 *  @brief: After this command is transmitted the buffer if zeroed
 */
void Epd::clearBuffer(void){
    memset(frame_buffer_black, 0X00, sizeof(frame_buffer_black));
}



void Epd::Sleep(void) {
    SendCommand(0x10);
    SendData(0x01);
}

// img - array containing image data
// topLeft_x - x cordinate of the top-left corner of image on screen 
// topLeft_y - y coordinate of the top-left corner of the image on screen
// xLen - bit length of image in the x direction
// yLen - bit length of image in the y direction
int8_t Epd::showImg(const uint8_t *img, uint16_t topLeft_x, uint16_t topLeft_y, uint16_t xLen, uint16_t yLen)
{
    if((topLeft_x + xLen) > SCREEN_WIDTH || (topLeft_y + yLen) > SCREEN_HEIGHT || topLeft_x < 0 || topLeft_y < 0){
        return -1; 
    };

    return copyImgToBuffer(0, topLeft_x, topLeft_y, img, xLen, yLen); 
}


int8_t Epd::copyImgToBuffer(size_t tableByteIdx, uint16_t topLeft_x, uint16_t topLeft_y, const uint8_t *img, uint16_t xLen, uint16_t yLen)
{ 
    uint16_t lineByteWidth = calcXByteLength(xLen);  // byte length of image in the x direction
    size_t bitPos = 0;  // for a 200x200 screen, this value is 0-39,999 
                        // and 0 corresponds to the top-left of the letter block
    for(uint8_t i=0; i < yLen; i++){
        bitPos = (topLeft_y * SCREEN_WIDTH) + topLeft_x;
        copyLineToBuffer(bitPos, xLen, img + tableByteIdx); 
        topLeft_y++;  // move to next line in the buffer 
        tableByteIdx += lineByteWidth;  // move to next line in the letter table 
    }
    
    return 1;
}



int8_t Epd::drawRect(uint16_t topLeft_x, uint16_t topLeft_y, uint16_t xLen, uint16_t yLen, bool black)
{
    if((topLeft_x + xLen) > SCREEN_WIDTH || (topLeft_y + yLen) > SCREEN_HEIGHT || topLeft_x < 0 || topLeft_y < 0){
        return -1; 
    };

    uint16_t xByteWidth = calcXByteLength(xLen); 
    uint8_t xLine[xByteWidth];
    for(size_t i=0; i<xByteWidth; i++)
    {
        if(black)
            xLine[i] = 0xFF;
        else
            xLine[i] = 0x00; 
    }

    size_t scrBitPos = 0;
    for(uint16_t j=0; j < yLen; j++)
    {
        scrBitPos = (topLeft_y * SCREEN_WIDTH) + topLeft_x;
        copyLineToBuffer(scrBitPos, xLen, xLine); 
        topLeft_y++;  // move to next line
    }

    return 1; 
}


/**
 *  @brief: Print string values onto the screen starting at the pixel location provided.
 *          (x,y) point refers the to the bottom-left corner of the first char in the string  
 *  strToPrint: string to print 
 *  xPos: x position of where the left side of the string should start - 0 indexed
 *  yPos: y posiiton of where the bottom side of the string should start - 0 indexed 
 */
int8_t Epd::printf(const char *strToPrint, font_t *scrnFont, uint16_t xPos, uint16_t yPos){
    // should have room to print at least 1 char
    if((xPos + scrnFont->charHeight) > SCREEN_WIDTH || xPos < 0 || yPos < 0){
        return -1; 
    };

    size_t idx = 0; 
    uint16_t xCursor = xPos; 
    uint16_t yCursor = yPos;
    while(strToPrint[idx] != '\0'){
        if(strToPrint[idx] == ' ')
        {
            yCursor += (scrnFont->spaceWidth) * 3; 
            idx++; 
            continue; 
        }
        uint16_t charWidth = getCharWidth(strToPrint[idx], scrnFont); 
        // Serial.printf("%c is %i wide\n", strToPrint[idx], charWidth); 
        // Serial.printf("yCursor: %i\n", yCursor);
        if((yCursor + charWidth) > SCREEN_HEIGHT)
            return -2;  // not enough space for this char
        copyCharToBuffer(strToPrint[idx], xCursor, yCursor, scrnFont);
        yCursor += charWidth + scrnFont->spaceWidth; 
        idx++;         
    }
    return 1; 
}


void Epd::updateScreen(bool forceFullRefresh)
{
    if(forceFullRefresh)
    {
        refreshCounter = PARTIAL_REFRESH_LIMIT; 
    }
    
    if(refreshCounter % PARTIAL_REFRESH_LIMIT == 0)
    {
        refreshCounter = 0; 
        ChangeRefreshMode(false); 
    }
    else if(refreshCounter % PARTIAL_REFRESH_LIMIT == 1)
    {
        ChangeRefreshMode(true); 
    }

    refreshCounter++; 
    DisplayFrame(frame_buffer_black); 
}


uint16_t Epd::getCharWidth(char thisChar, font_t *scrnFont)
{
    uint8_t charIdx = thisChar - scrnFont->startChar;
    return scrnFont->charDescs[charIdx].width; 
}

// returns the location of the first byte in the font table of the 
// queried char 
size_t Epd::getAsciiTableIdx(const char thisChar, font_t *scrnFont){
    uint8_t charIdx = thisChar - scrnFont->startChar; 
    return scrnFont->charDescs[charIdx].offset; 
}


void Epd::copyCharToBuffer(char charToPrint, uint16_t xCursor, uint16_t yCursor, font_t *scrnFont){
    size_t tableByteIdx = getAsciiTableIdx(charToPrint, scrnFont); 
    uint16_t charYLen = getCharWidth(charToPrint, scrnFont); 

    copyImgToBuffer(tableByteIdx, xCursor, yCursor, scrnFont->bitmap, scrnFont->charHeight, charYLen);
}

// return width in bytes of the data array who's bit len is xBitLen 
uint16_t Epd::calcXByteLength(size_t xBitLen){
    return (xBitLen % 8 == 0)? (xBitLen / 8 ): (xBitLen / 8 + 1);  // num bytes used in font table for 1 line of a letter
}


// bitPos - bit position of the screen where line is to start
// xBitLen - length of the line in bits in the screen's x direction
// lineData - start of the image line data
// Desc - this function copies a single line of data onto screen buffer by going each data byte at a time
void Epd::copyLineToBuffer(size_t bitPos, size_t xBitLen, const uint8_t *lineData){
    uint16_t byteWidth = calcXByteLength(xBitLen); 
    size_t imgByteIdx = bitPos >> 3;
    size_t imgBitIdx = bitPos % 8; 
    int xWidthLeft = xBitLen; 
    size_t tableByteIdx = 0; 

    //Single line may consist of multiple bytes 
    for(uint16_t i=0; i<byteWidth; i++){
        if(imgByteIdx >= (this->width * this->height)/8){  // make sure within bounds of frame_buffer
            continue; 
        }

        uint8_t mask1 = 0xFF;
        if(imgBitIdx == 0)
            mask1 = 0x00;
        else
            mask1 <<= (8 - imgBitIdx);  // ex: 10000000 0 out bits for new data

        size_t endBitIdx = imgBitIdx + xWidthLeft - 1;   
        uint8_t copyByte = lineData[tableByteIdx] >> imgBitIdx; 
        //check for when have created more 0s than you need (edge case when line ends before end of screen byte)
        //don't want to 0 out more than width of line
        if(endBitIdx < 7){ 
            uint8_t undo0Mask = 0xFF;
            undo0Mask >>= (endBitIdx + 1);  // ex: 00000011
            mask1 |= undo0Mask;  // ex: 10000011 
            copyByte &= ~undo0Mask; // 0 out right end of copyByte so don't override buffer data with 1s 
        } 

        frame_buffer_black[imgByteIdx] &= mask1; //make 0s on bits where the char line will go
        frame_buffer_black[imgByteIdx] |= copyByte; 
        xWidthLeft -= (8 - imgBitIdx);  
        imgByteIdx++;


        //if only part of data byte taken before, then finish off the rest of data byte 
        //imgBitIdx also represents number of data bits pushed into next byte
        if(imgBitIdx > 0 && xWidthLeft > 0){
            uint8_t mask2 = ~mask1; 
            copyByte = lineData[tableByteIdx] << (8-imgBitIdx); 

            //take into account case when xWidthLeft ends before finishing up second part of data byte 
            if(imgBitIdx > xWidthLeft){
                // have some extraneous 0 bits that you can't include
                // size_t numExtraZeros = imgBitIdx - xWidthLeft; 
                mask2 = 0xFF; 
                // mask2 >>= numExtraZeros;
                mask2 >>= xWidthLeft; 
                copyByte &= ~mask2; 
            }
            
            frame_buffer_black[imgByteIdx] &= mask2; 
            frame_buffer_black[imgByteIdx] |= copyByte;
            xWidthLeft -= imgBitIdx;  
        }
        tableByteIdx++; 
    }
}




/* END OF FILE */


