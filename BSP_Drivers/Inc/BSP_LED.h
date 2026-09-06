#ifndef __BSP_LED_H
#define __BSP_LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <stdbool.h>

typedef enum 
{
    ALL = 0,
    LED1,
    LED2,
    LED3,
    LED4
} LED_t;

void BSP_LED_ON(LED_t led);
void BSP_LED_OFF(LED_t led);
void LED_Blink(LED_t led);
void LED_Blink2(LED_t led, uint8_t repeat, uint16_t delay);

#ifdef __cplusplus
}
#endif


#endif