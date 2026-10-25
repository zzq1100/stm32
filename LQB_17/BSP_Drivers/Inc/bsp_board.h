#ifndef BSP_BOARD_H
#define BSP_BOARD_H
#include <stdint.h>
void BSP_BoardInit(void);
uint8_t BSP_ReadKeys(void); /* bit0..3 对应 B1..B4，1 为按下 */
uint8_t BSP_ReadVoltage(float *voltage);
float BSP_ReadFrequency(uint32_t now);
void BSP_SetDuty(uint16_t duty); /* 单位 0.01%，范围 500..9500 */
void BSP_SetLEDs(uint8_t mask); /* bit0..7 对应 LD1..LD8，1 为亮 */
#endif
