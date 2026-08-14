#include "BSP_LED.h"


void BSP_LED_Control(LED_t led, bool flag)
{
    GPIO_PinState state = flag ? GPIO_PIN_RESET : GPIO_PIN_SET;
    switch (led)
    {
    case LED1:
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, state);
        break;
    case LED2:
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, state);
        break;
    case LED3:
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, state);
        break;
    case LED4:
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_11, state);
        break;
    default:
        break;
    }
}

void BSP_LED_ON(LED_t led)
{
    BSP_LED_Control(led, true);
}

void BSP_LED_OFF(LED_t led)
{
    BSP_LED_Control(led, false);
}