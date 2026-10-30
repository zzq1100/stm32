#ifndef BSP_LCD_H
#define BSP_LCD_H

#include <stdint.h>

#define BSP_LCD_ROWS 10U
#define BSP_LCD_COLS 20U

/* 调用前先初始化 LED 并让 PD2 保持低，避免 LCD 初始化改变灯状态。 */
void BSP_LCD_Init(void);

/* 清空“准备显示”的字符缓存，不立即整屏清屏。 */
void BSP_LCD_ClearBuffer(void);

/* row 从 0 开始；最多取 20 个字符，不足补空格。
 * 非可打印 ASCII 替换为 '?'；越界行或空指针被忽略。
 * 这里只改缓存，调用者的字符串可以是局部数组。 */
void BSP_LCD_SetLine(uint8_t row, const char *text);

/* 每次主循环最多绘制一个发生变化的字符，避免长期阻塞其它任务。
 * 本模块与 LED 均只在主循环使用，不允许中断中访问 LCD。 */
void BSP_LCD_Service(void);

#endif
