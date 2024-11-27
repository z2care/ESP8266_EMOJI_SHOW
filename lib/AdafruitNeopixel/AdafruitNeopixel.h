#ifndef _Adafruit_NeoPixel_H
#define _Adafruit_NeoPixel_H
 
#include "c_types.h"
//#include "user_interface.h"
//#include "ets_sys.h"
#include "gpio.h"
 
//thanks for https://github.com/cnlohr/ws2812esp8266 
//thanks for https://github.com/adafruit/Adafruit_NeoPixel
 
#define WSGPIO 0 //must use the ESP8266 GPIO0 as the pin to drive WS2812B RGB LED!!!
//user can change
#define PIXEL_MAX 12//the total numbers of LEDs you are used in your project
 
//You will have to 	os_intr_lock();  	os_intr_unlock();
 
//void setAllColor(uint8_t r, uint8_t g, uint8_t b);
void setOneColor(uint16_t n, uint32_t c);
void setAllColor(uint32_t c);
void cleanOneColor(uint16_t n, uint32_t c);
void cleanAllColor(void);
 
void setAllPixel(void);
 
uint32_t Color(uint8_t r, uint8_t g, uint8_t b);
uint32_t Wheel(uint8_t WheelPos);
void rainbowCycle(uint8_t wait) ;
void theaterChase(uint32_t c, uint8_t wait);
void theaterChaseRainbow(uint8_t wait);
void colorWipe(uint32_t c, uint8_t wait);
void WS2812B_Test(void);
void ICACHE_FLASH_ATTR WS2812B_Init(void);
 
uint32_t changeL(uint32_t rgb, float k);
 
#endif
