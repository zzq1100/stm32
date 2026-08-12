#ifndef __BSP_USART_REDIR
#define __BSP_USART_REDIR


#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "stm32f1xx_hal.h"

extern UART_HandleTypeDef *ghusart;

void BSP_Usart_Init(UART_HandleTypeDef *husart);
int fputc(int ch, FILE *fp);
int fgetc(FILE *fp);

#ifdef __cplusplus
}
#endif


#endif