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

void LED_Blink(LED_t led)
{
    if (led == LED1)
    {
        for (int i = 0; i < 3; i++)
        {
            BSP_LED_ON(LED1);
            HAL_Delay(500);
            BSP_LED_OFF(LED1);
            HAL_Delay(500);
        }
    }
    if (led == LED2)
    {
        for (int i = 0; i < 3; i++)
        {
            BSP_LED_ON(LED2);
            HAL_Delay(500);
            BSP_LED_OFF(LED2);
            HAL_Delay(500);
        }
    }
    if (led == LED3)
    {
        for (int i = 0; i < 3; i++)
        {
            BSP_LED_ON(LED3);
            HAL_Delay(500);
            BSP_LED_OFF(LED3);
            HAL_Delay(500);
        }
    }
    if (led == LED4)
    {
        for (int i = 0; i < 3; i++)
        {
            BSP_LED_ON(LED4);
            HAL_Delay(500);
            BSP_LED_OFF(LED4);
            HAL_Delay(500);
        }
    }
    if (led == ALL)
    {
        LED_t leds[4] = {LED1, LED2, LED3, LED4};
        int cnt = 3;
        while (cnt--)
        {
            for (int i = 0; i < 4; i++)
            {
                BSP_LED_ON(leds[i]);
            }
            HAL_Delay(500);
            for (int i = 0; i < 4; i++)
            {
                BSP_LED_OFF(leds[i]);
            }
            HAL_Delay(500);
        }
    }
}

void LED_Blink2(LED_t led, uint8_t repeat, uint16_t delay)
{
    if (led == LED1)
    {
        for (int i = 1; i <= repeat; i++)
        {
            BSP_LED_ON(LED1);
            printf("第 %d 次闪灯\n", i);
            HAL_Delay(delay);
            BSP_LED_OFF(LED1);
            HAL_Delay(delay);
        }
    }
    if (led == LED2)
    {
        for (int i = 1; i <= repeat; i++)
        {
            BSP_LED_ON(LED2);
            printf("第 %d 次闪灯\n", i);
            HAL_Delay(delay);
            BSP_LED_OFF(LED2);
            HAL_Delay(delay);
        }
    }
    if (led == LED3)
    {
        for (int i = 1; i <= repeat; i++)
        {
            BSP_LED_ON(LED3);
            printf("第 %d 次闪灯\n", i);
            HAL_Delay(delay);
            BSP_LED_OFF(LED3);
            HAL_Delay(delay);
        }
    }
    if (led == LED4)
    {
        for (int i = 1; i <= repeat; i++)
        {
            BSP_LED_ON(LED4);
            printf("第 %d 次闪灯\n", i);
            HAL_Delay(delay);
            BSP_LED_OFF(LED4);
            HAL_Delay(delay);
        }
    }
    if (led == ALL)
    {
        LED_t leds[4] = {LED1, LED2, LED3, LED4};
        int cnt = repeat;
        int led_times = 1;
        while (cnt--)
        {
            for (int i = 0; i < 4; i++)
            {
                BSP_LED_ON(leds[i]);
            }
            printf("第 %d 次闪灯\n", led_times++);
            HAL_Delay(delay);
            for (int i = 0; i < 4; i++)
            {
                BSP_LED_OFF(leds[i]);
            }
            HAL_Delay(delay);
        }
    }
}