# GDEW0154Z04 2.9" e-paper library for Arduino
This is a library for the Waveshare 2.9" e-Paper display. The library allows you to 

## Hardware connection (e-Paper --> Arduino)
    3.3V --> 3V3
    GND  --> GND
    DIN  --> D11
    CLK  --> D13
    CS   --> D10
    DC   --> D9
    RST  --> D8
    BUSY --> D7


![e-paper display](http://www.waveshare.com/img/devkit/general/e-Paper-Modules-CMP.jpg)
# Interfaces
| Name | Description                                                   |
|------|---------------------------------------------------------------|
| VCC  | 3.3V                                                          |
| GND  | GND                                                           |
| DIN  | SPI MOSI                                                      |
| CLK  | SPI SCK                                                       |
| CS   | SPI chip select (Low active)                                  |
| DC   | Data/Command control pin (High for data, and low for command) |
| RST  | External reset pin (Low for reset)                            |
| BUSY | Busy state output pin (Low for busy)                          |




