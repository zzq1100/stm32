#ifndef __BSP_USART_REDIR_H
#define __BSP_USART_REDIR_H // 防止头文件被重复包含



#ifdef __cplusplus
extern "C" { // extern "C", 告诉编译器，头文件里的函数是用C语言规则编译的
#endif


#include <stdio.h>
#include "stm32f1xx_hal.h"

extern UART_HandleTypeDef *ghusart;

void BSP_Usart_Init(UART_HandleTypeDef *husart);



#ifdef __cplusplus
}
#endif


#endif