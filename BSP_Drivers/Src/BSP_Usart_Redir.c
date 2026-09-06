#include "BSP_Usart_Redir.h"

UART_HandleTypeDef *ghusart = NULL;

void BSP_Usart_Init(UART_HandleTypeDef *husart)
{
    ghusart = husart;
}

int fputc(int ch, FILE *fp)
{
    UNUSED(fp);
    if (NULL != ghusart)
    {
        if (HAL_UART_Transmit(ghusart, (uint8_t *)&ch, 1, HAL_MAX_DELAY) == HAL_OK)
        {
            return ch;
        }
        return EOF;
    }
    else
        return EOF;
}

int fgetc(FILE *fp)
{
    UNUSED(fp);
    if (NULL != ghusart)
    {
        uint8_t ch;
        while (1)
        {
            if (HAL_UART_Receive(ghusart, &ch, 1, HAL_MAX_DELAY) != HAL_OK)
            {
                return EOF;
            }
            // 读取成功了一个字符
            if (ch != '\r')
            {
                return (int)ch;
            }
        }
    }
    else
        return EOF;
}