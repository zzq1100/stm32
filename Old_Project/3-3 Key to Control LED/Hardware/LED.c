#include "stm32f10x.h"

void LED_Init()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void LED_ON(int led)
{
    if (led == 0)
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_0);
    }
    else
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_1);
    }
}

void LED_OFF(int led)
{
    if (led == 0)
    {
        GPIO_SetBits(GPIOA, GPIO_Pin_0);
    }
    else
    {
        GPIO_SetBits(GPIOA, GPIO_Pin_1);
    }
}

void LED_Turn(int led)
{
    if (led == 0)
    {
        if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_0) == 0)
        {
            GPIO_SetBits(GPIOA, GPIO_Pin_0);
        }
        else
        {
            GPIO_ResetBits(GPIOA, GPIO_Pin_0);
        }
    }
    if (led == 1)
    {
        if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_1) == 0)
        {
            GPIO_SetBits(GPIOA, GPIO_Pin_1);
        }
        else
        {
            GPIO_ResetBits(GPIOA, GPIO_Pin_1);
        }
    }
}
