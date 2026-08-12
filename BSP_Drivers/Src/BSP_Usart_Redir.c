#include "BSP_Usart_Redir.h"

UART_HandleTypeDef *ghusart;

void BSP_Usart_Init(UART_HandleTypeDef *husart)
{
    ghusart = husart;
}

int fputc(int ch, FILE *fp)
{
  UNUSED(fp);
  if (HAL_UART_Transmit(ghusart, (uint8_t*)&ch, 1, HAL_MAX_DELAY) == HAL_OK)
  {
    return ch;
  }
  return EOF;
}

int fgetc(FILE *fp)
{
  UNUSED(fp);
  uint8_t ch = 0;
  while (1)
  {
    if (HAL_UART_Receive(ghusart, &ch, 1, HAL_MAX_DELAY) != HAL_OK)
    {
      return EOF;
    }
    if (ch != '\r')
    {
      return (int)ch;
    }
  }
}