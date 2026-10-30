#include "bsp_led.h"
#include "main.h"

static uint8_t last_mask;
static uint8_t has_written;

static void Latch_Wait(void)
{
    volatile uint32_t i;
    /* 给 573 的建立、脉宽和保持时间留余量。
     * volatile 保留循环，NOP 保留空操作；这是短暂总线等待，不是毫秒延时。 */
    for (i = 0U; i < 16U; ++i) {
        __NOP();
    }
}

void BSP_LED_Init(void)
{
    has_written = 0U;
    HAL_GPIO_WritePin(LED_LE_GPIO_Port, LED_LE_Pin, GPIO_PIN_RESET);
    BSP_LED_Write(0U);
}

void BSP_LED_Write(uint8_t mask)
{
    uint32_t saved;
    uint32_t led_bus;
    uint8_t active_low;

    /* 锁存器会保持旧输出；状态没变时无需重复占用 LCD 总线。 */
    if (has_written && mask == last_mask) {
        return;
    }

    saved = GPIOC->ODR;
    active_low = (uint8_t)(~mask);  /* LED 低电平亮：逻辑“亮”转换为 0。 */
    led_bus = (uint32_t)active_low << 8;
    GPIOC->ODR = (saved & 0x00FFU) | led_bus;
    __DSB();                       /* 确保数据口写入完成后再打开锁存器。 */
    Latch_Wait();

    HAL_GPIO_WritePin(LED_LE_GPIO_Port, LED_LE_Pin, GPIO_PIN_SET);
    __DSB();
    Latch_Wait();                   /* LE 高期间，输出跟随数据。 */
    HAL_GPIO_WritePin(LED_LE_GPIO_Port, LED_LE_Pin, GPIO_PIN_RESET);
    __DSB();
    Latch_Wait();                   /* LE 变低后保持数据，再归还总线。 */

    GPIOC->ODR = saved;
    last_mask = mask;
    has_written = 1U;
}
