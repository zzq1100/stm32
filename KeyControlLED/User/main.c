#include "stm32f10x.h" // Device header
#include "Delay.h"
#include "LED.h"
#include "Key.h"

uint8_t g_KeyNum;

int main()
{
	Key_Init();
	LED_Init();	
	while (1)
	{
		g_KeyNum = Key_GetNum();
		if (g_KeyNum == 0)
		{
			LED0_ON();
		}
		if (g_KeyNum == 1)
		{
			LED0_OFF();
		}
	}
}

