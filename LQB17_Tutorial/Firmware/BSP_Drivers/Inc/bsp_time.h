#ifndef BSP_TIME_H
#define BSP_TIME_H

#include <stdint.h>

typedef struct {
    uint32_t last_ms;               /* 最近一次已处理的周期边界，单位 ms。 */
} BSP_Periodic;

/* timer 必须指向有效对象；不同任务必须拥有各自的 timer。 */
void BSP_Periodic_Init(BSP_Periodic *timer, uint32_t now_ms);

/* 到期返回 1，并前进一个周期；未到期或 period_ms=0 返回 0。
 * 超期多轮时每次只补一个周期，调用者仍能轮流服务其它任务。
 * 调用间隔不得跨越完整的 uint32_t 毫秒回绕周期（约 49.7 天）。
 * 调用者不应长期阻塞；补调度无法重建错过的历史传感器样本。 */
uint8_t BSP_Periodic_Due(BSP_Periodic *timer,
                         uint32_t now_ms, uint32_t period_ms);

#endif
