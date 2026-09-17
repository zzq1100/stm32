#include "stm32f10x.h" // Device header
#include "Delay.h"

int main()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	
	while (1)
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_0);
		// GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET);
		Delay_ms(500);
		// GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_SET);
		GPIO_SetBits(GPIOA, GPIO_Pin_0);
		Delay_ms(500);
	}
	
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
//	
//	GPIO_InitTypeDef GPIO_InitStruct;
//	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
//	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12;
//	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(GPIOB, &GPIO_InitStruct);
//	
//	
//	while (1)
//	{
//		GPIO_ResetBits(GPIOB, GPIO_Pin_12);
//		// GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET);
//		Delay_ms(500);
//		// GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_SET);
//		GPIO_SetBits(GPIOB, GPIO_Pin_12);
//		Delay_ms(500);
//	}
}

