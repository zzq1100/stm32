#include "bsp_capture.h"
#include "tim.h"

static volatile uint32_t last_edge_ms, published_ticks, overcaptures;
static volatile uint16_t published_periods;
static volatile uint8_t published_valid, started;
/* 以下工作区只有捕获回调使用，初始化在开中断以前完成。 */
static uint16_t previous, window_periods;
static uint32_t window_ticks;
static uint8_t have_previous;

static void ResetWindow(void)
{
    window_periods = 0U;
    window_ticks = 0U;
}

uint8_t BSP_Capture_Init(void)
{
    started = published_valid = have_previous = 0U;
    published_ticks = last_edge_ms = overcaptures = 0U;
    published_periods = previous = 0U;
    ResetWindow();
    if (htim3.Init.Prescaler != 79U || htim3.Init.Period != 65535U) return 0U;
    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_CC1 | TIM_FLAG_CC1OF);
    started = 1U;
    if (HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1) != HAL_OK) {
        started = 0U;
    }
    return started;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    uint16_t current, delta;
    uint32_t now;
    uint8_t stale;
    if (htim->Instance != TIM3 || htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1 || !started) return;

    current = (uint16_t)HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
    now = HAL_GetTick();
    stale = (uint8_t)((uint32_t)(now - last_edge_ms) >= BSP_CAPTURE_TIMEOUT_MS);
    last_edge_ms = now;

    if (__HAL_TIM_GET_FLAG(htim, TIM_FLAG_CC1OF) != RESET) {
        /* CCR被新边沿覆盖，旧周期不可信：丢弃窗口，重新找一对边沿。 */
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_CC1OF);
        ++overcaptures;
        published_valid = 0U;
        ResetWindow();
        previous = current;
        have_previous = 1U;
        return;
    }
    if (!have_previous || stale) {
        previous = current;
        have_previous = 1U;
        published_valid = 0U;
        ResetWindow();
        return;                    /* 一个边沿只能确定起点，还没有周期。 */
    }

    /* ARR=65535，且两边沿间隔<50ms，最多跨一次16位计数回绕。 */
    delta = (uint16_t)(current - previous);
    previous = current;
    if (delta == 0U) {
        published_valid = 0U;      /* 不对零周期做除法。 */
        ResetWindow();
        return;
    }
    if (delta > 1250U) {
        /* 周期>1250us已低于800Hz，立即发布，避免慢等16个低频周期。 */
        published_ticks = delta;
        published_periods = 1U;
        published_valid = 1U;
        ResetWindow();
        return;
    }

    window_ticks += delta;
    ++window_periods;
    if (window_periods == BSP_CAPTURE_PERIODS) {
        published_ticks = window_ticks;
        published_periods = window_periods;
        published_valid = 1U;
        ResetWindow();
    }
    /* 中断只做整数记录，除法、LCD和LED都留给主循环。 */
}

void BSP_Capture_Read(BSP_CaptureSample *sample)
{
    uint32_t irq_state, now, last;
    uint8_t valid;
    if (sample == 0) return;

    irq_state = __get_PRIMASK();
    __disable_irq();
    /* 多字段必须来自同一次发布；volatile本身不能保证这一点。 */
    sample->period_ticks = published_ticks;
    sample->periods = published_periods;
    sample->overcaptures = overcaptures;
    valid = (uint8_t)(started && published_valid);
    last = last_edge_ms;
    now = HAL_GetTick();           /* 与last在同一临界区读取，避免旧now减新last。 */
    __set_PRIMASK(irq_state);      /* 恢复进入前状态，不能无条件开启中断。 */

    sample->frequency_hz = 0.0f;
    sample->valid = 0U;
    if (valid && (uint32_t)(now - last) < BSP_CAPTURE_TIMEOUT_MS && sample->period_ticks != 0U) {
        sample->frequency_hz = (float)BSP_CAPTURE_CLOCK_HZ * sample->periods / sample->period_ticks;
        sample->valid = 1U;
    }
}
