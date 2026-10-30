#ifndef BSP_CAPTURE_H
#define BSP_CAPTURE_H

#include <stdint.h>

#define BSP_CAPTURE_CLOCK_HZ 1000000U
#define BSP_CAPTURE_PERIODS 16U
#define BSP_CAPTURE_TIMEOUT_MS 50U

typedef struct {
    float frequency_hz;            /* 未按800/8000限幅的原始频率。 */
    uint32_t period_ticks;         /* 本次发布窗口累计的1us计数。 */
    uint32_t overcaptures;         /* 硬件捕获覆盖次数，调试用。 */
    uint16_t periods;              /* 窗口周期数，常规16，低频快速更新时1。 */
    uint8_t valid;                 /* 已有完整测量，且50ms内仍有边沿。 */
} BSP_CaptureSample;

/* MX_TIM3_Init后调用一次；启用CH1捕获中断，成功返回1。 */
uint8_t BSP_Capture_Init(void);
/* 主循环读取一致快照；函数内部读取tick，避免传入已落后的now。
 * 无信号/启动等待/捕获覆盖时valid=0，frequency_hz=0。 */
void BSP_Capture_Read(BSP_CaptureSample *sample);

#endif
