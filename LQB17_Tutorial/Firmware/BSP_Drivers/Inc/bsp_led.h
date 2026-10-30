#ifndef BSP_LED_H
#define BSP_LED_H

#include <stdint.h>

/* 前提：CubeMX 生成的 MX_GPIO_Init 已配置 PC8~PC15、PD2。
 * 初始化主动锁存全灭，不能依赖外部锁存器的上电内容。 */
void BSP_LED_Init(void);

/* bit0~bit7 对应 LD1~LD8：1 表示希望亮，0 表示希望灭。
 * 只能在主循环调用；LCD 和 LED 共用 PC 口，不能从中断并发访问。
 * 竞赛业务层必须保持 bit4~bit7 为 0。 */
void BSP_LED_Write(uint8_t mask);

#endif
