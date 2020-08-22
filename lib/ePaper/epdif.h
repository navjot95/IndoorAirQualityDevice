
#ifndef EPDIF_H
#define EPDIF_H

#include <Arduino.h>

// Pin definition
#define RST_PIN         27
#define DC_PIN          33
#define CS_PIN          14
#define BUSY_PIN        13

class EpdIf {
public:
    EpdIf(void);
    ~EpdIf(void);

    static int  IfInit(void);
    static void DigitalWrite(int pin, int value); 
    static int  DigitalRead(int pin);
    static void DelayMs(unsigned int delaytime);
    static void SpiTransfer(unsigned char data);
};

#endif
