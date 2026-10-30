#ifndef TEST_LCD_H
#define TEST_LCD_H
#include <stdint.h>
#define Black 0x0000U
#define White 0xFFFFU
void LCD_Init(void);
void LCD_SetBackColor(uint16_t color);
void LCD_SetTextColor(uint16_t color);
void LCD_Clear(uint16_t color);
void LCD_DisplayChar(uint8_t line, uint16_t column, uint8_t ascii);
#endif
