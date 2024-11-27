#include "AdafruitNeoPixel.h"
//#include "ets_sys.h"
//#include "osapi.h"
/*
 #define GPIO_OUTPUT_SET(gpio_no, bit_value) \
	gpio_output_set(bit_value<<gpio_no, ((~bit_value)&0x01)<<gpio_no, 1<<gpio_no,0)
 */
 
//I just used a scope to figure out the right time periods.
void SEND_WS_0() {
	uint8_t time;
	time = 3;
	while (time--)
		WRITE_PERI_REG(PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 1);
	time = 7;
	while (time--)
		WRITE_PERI_REG(PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 0);
}
 
void SEND_WS_1() {
	uint8_t time;
	time = 7;
	while (time--)
		WRITE_PERI_REG(PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 1);
	time = 3;
	while (time--)
		WRITE_PERI_REG(PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 0);
}
 
void // ICACHE_FLASH_ATTR
WS2812Send_8bit(uint8_t dat) {
	uint16_t i;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(WSGPIO), 0);
	ets_intr_lock();
	uint8_t mask = 0x80;
	uint8_t byte = dat;
	while (mask) {
		if (byte & mask)
			SEND_WS_1();
		else
			SEND_WS_0();
		mask >>= 1;
	}
 
	ets_intr_unlock();
}
 
//GRB format,MSB firsr.
void //ICACHE_FLASH_ATTR
WS2812BSend_24bit(uint8_t R, uint8_t G, uint8_t B) {
	WS2812Send_8bit(G);
	WS2812Send_8bit(R);
	WS2812Send_8bit(B);
}
//delay for millisecond
void HAL_Delay(int time) {
	os_delay_us(time * 1000);
}
 
static uint8_t rBuffer[PIXEL_MAX] = { 0 };
static uint8_t gBuffer[PIXEL_MAX] = { 0 };
static uint8_t bBuffer[PIXEL_MAX] = { 0 };
 
void setOneColor(uint16_t n, uint32_t c) {
	rBuffer[n] = (uint8_t) (c >> 16);
	gBuffer[n] = (uint8_t) (c >> 8);
	bBuffer[n] = (uint8_t) c;
}
 
void setAllColor(uint32_t c) {
	uint8_t r, g, b;
	r = (uint8_t) (c >> 16);
	g = (uint8_t) (c >> 8);
	b = (uint8_t) c;
	uint8_t i;
	for (i = 0; i < PIXEL_MAX; i++) {
		rBuffer[i] = r;
		gBuffer[i] = g;
		bBuffer[i] = b;
	}
}
 
void cleanOneColor(uint16_t n, uint32_t c) {
	rBuffer[n] = 0;
	gBuffer[n] = 0;
	bBuffer[n] = 0;
}
 
void cleanAllColor() {
	uint8_t i;
	for (i = 0; i < PIXEL_MAX; i++) {
		rBuffer[i] = 0;
		gBuffer[i] = 0;
		bBuffer[i] = 0;
	}
}
 
void setAllPixel(void) {
	uint8_t i;
	for (i = 0; i < PIXEL_MAX; i++) {
		WS2812BSend_24bit(rBuffer[i], gBuffer[i], bBuffer[i]);
	}
}
 
uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t) r << 16) | ((uint32_t) g << 8) | b;
}
uint32_t Wheel(uint8_t WheelPos) {
	WheelPos = 255 - WheelPos;
	if (WheelPos < 85) {
		return Color(255 - WheelPos * 3, 0, WheelPos * 3);
	}
	if (WheelPos < 170) {
		WheelPos -= 85;
		return Color(0, WheelPos * 3, 255 - WheelPos * 3);
	}
	WheelPos -= 170;
	return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
 
void rainbow(uint8_t wait) {
	uint16_t i, j;
	for (j = 0; j < 256; j++) {
		for (i = 0; i < PIXEL_MAX; i++) {
			setOneColor(i, Wheel((i + j) & 255));
		}
		HAL_Delay(wait);
	}
	setAllPixel();
}
// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
	uint16_t i, j;
 
	for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
		for (i = 0; i < PIXEL_MAX; i++) {
			setOneColor(i, Wheel(((i * 256 / PIXEL_MAX) + j) & 255));
		}
		HAL_Delay(wait);
	}
	setAllPixel();
}
//Theatre-style crawling lights.o??¡§????
void theaterChase(uint32_t c, uint8_t wait) {
	int i, j, q;
	for (j = 0; j < 10; j++) {  //do 10 cycles of chasing
		for (q = 0; q < 3; q++) {
			for (i = 0; i < PIXEL_MAX; i = i + 1)  //turn every one pixel on
					{
				setOneColor(i + q, c);
			}
			HAL_Delay(wait);
 
			for (i = 0; i < PIXEL_MAX; i = i + 1) //turn every one pixel off
					{
				setOneColor(i + q, 0);
			}
		}
	}
	setAllPixel();
}
 
//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
	int i, j, q;
	for (j = 0; j < 256; j++) {     // cycle all 256 colors in the wheel
		for (q = 0; q < 3; q++) {
			for (i = 0; i < PIXEL_MAX; i = i + 1) //turn every one pixel on
					{
				setOneColor(i + q, Wheel((i + j) % 255));
			}
 
			HAL_Delay(wait);
 
			for (i = 0; i < PIXEL_MAX; i = i + 1) //turn every one pixel off
					{
				setOneColor(i + q, 0);
			}
		}
	}
	setAllPixel();
}
// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
	uint16_t i = 0;
	for (i = 0; i < PIXEL_MAX; i++) {
		setOneColor(i, c);
		HAL_Delay(wait);
	}
}
 
void //ICACHE_FLASH_ATTR
WS2812B_Init(void) {
	setAllColor(0);
	setAllPixel();
}
 
void WS2812B_Test(void) {
	//HAL_Delay(500);
	// rainbowCycle(20);
	// theaterChaseRainbow(50);
}
