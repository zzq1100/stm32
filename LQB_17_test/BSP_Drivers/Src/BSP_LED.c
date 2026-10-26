#include "BSP_LED.h"

void BSP_SetLEDs(uint8_t mask)
{
    uint32_t saved = GPIOC->ODR;   /*保存原来输出寄存器的配置*/

    GPIOC->ODR = (saved & 0x00ffU) /*低八位保持不变，高八位清零*/
                | ((uint32_t)(uint8_t)(~mask) << 8);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);   /*打开透明锁存*/
    __NOP();__NOP();__NOP();
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET); /*关闭锁存，锁住led数据*/

    GPIOC->ODR = saved;   /*将LCD的数据还原，不影响led数据*/
}
