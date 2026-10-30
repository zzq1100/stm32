#include "bsp_time.h"

void BSP_Periodic_Init(BSP_Periodic *timer, uint32_t now_ms)
{
    timer->last_ms = now_ms;
}

uint8_t BSP_Periodic_Due(BSP_Periodic *timer,
                         uint32_t now_ms, uint32_t period_ms)
{
    if (period_ms == 0U) {
        return 0U;
    }

    /* 无符号差值允许计数从 0xFFFFFFFF 回到 0 后继续计时。 */
    if ((uint32_t)(now_ms - timer->last_ms) < period_ms) {
        return 0U;
    }

    /* 推进约定边界，避免每次晚一点执行就把误差永久积累。 */
    timer->last_ms += period_ms;
    return 1U;
}
