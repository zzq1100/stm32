/* 第三阶段验证入口，不是竞赛最终app。
 * B1/B2直接加减PWM用于测驱动；竞赛的预设提交和渐变在后续实现。 */
#include "app.h"
#include "main.h"
#include "tim.h"
#include "bsp_led.h"
#include "bsp_lcd.h"
#include "bsp_time.h"
#include "bsp_keys.h"
#include "bsp_adc.h"
#include "bsp_capture.h"
#include "bsp_pwm.h"
#include "measurement.h"
#include <stdio.h>

BSP_ADC_Sample phase3_adc;
BSP_CaptureSample phase3_capture;
volatile float phase3_pressure, phase3_flow, phase3_offset;
volatile uint32_t phase3_event_count, phase3_last_held_ms, phase3_max_loop_gap_ms;
volatile uint8_t phase3_last_key, phase3_last_type, phase3_start_errors;
volatile uint8_t phase3_zero_result; /* 0未请求，1成功，2满量程拒绝，3采样失败。 */
static BSP_Keys keys;
static BSP_Periodic key_timer, sample_timer, screen_timer;
static uint16_t requested_duty;
static uint32_t previous_ms;

static void HandleEvent(const BSP_KeyEvent *event)
{
    float offset;
    ++phase3_event_count;
    phase3_last_key = event->key;
    phase3_last_type = (uint8_t)event->type;
    phase3_last_held_ms = event->held_ms;

    if (event->type == BSP_KEY_PRESS) {
        if (event->key == 1U && requested_duty < 9500U) requested_duty += 500U;
        if (event->key == 2U && requested_duty > 500U) requested_duty -= 500U;
        BSP_PWM_SetDuty(requested_duty);
    } else if (event->key == 3U && event->held_ms >= 2000U) {
        /* 必须松开后触发，并重新取电压；不使用按下时或上一帧的样本。 */
        if (BSP_ADC_Read(&phase3_adc)) {
            offset = phase3_offset;
            phase3_zero_result = Measurement_Calibrate(phase3_adc.voltage_v, &offset) ? 1U : 2U;
            phase3_offset = offset;
        } else {
            phase3_zero_result = 3U;
        }
    }
    /* B4只观察按键事件；本阶段还没有累计流量，不伪装成已经能清Q。 */
}

static void ComposeScreen(void)
{
    char line[21];
    unsigned v = (unsigned)(phase3_adc.voltage_v * 100.0f + 0.5f);
    unsigned p = (unsigned)(phase3_pressure * 10.0f + 0.5f);
    unsigned f = (unsigned)(phase3_flow * 10.0f + 0.5f);
    unsigned z = (unsigned)(phase3_offset * 100.0f + 0.5f);
    unsigned d = BSP_PWM_GetDuty();

    (void)snprintf(line, sizeof(line), "V=%u.%02u P=%u.%u", v/100U, v%100U, p/10U, p%10U);
    BSP_LCD_SetLine(3U, line);
    (void)snprintf(line, sizeof(line), "f=%lu F=%u.%u%c",
                   (unsigned long)(phase3_capture.frequency_hz + 0.5f), f/10U, f%10U,
                   Measurement_Overrange(phase3_capture.frequency_hz) ? '!' : ' ');
    BSP_LCD_SetLine(4U, line);
    (void)snprintf(line, sizeof(line), "D=%u.%02u C=%lu", d/100U, d%100U,
                   (unsigned long)__HAL_TIM_GET_COMPARE(&htim2, TIM_CHANNEL_2));
    BSP_LCD_SetLine(5U, line);
    if (phase3_event_count == 0U) {
        BSP_LCD_SetLine(6U, "KEY=NONE"); /* 尚无事件时不显示不存在的B0松开。 */
    } else {
        (void)snprintf(line, sizeof(line), "B%u %s %lums", (unsigned)phase3_last_key,
                       phase3_last_type == BSP_KEY_PRESS ? "DN" : "UP", (unsigned long)phase3_last_held_ms);
        BSP_LCD_SetLine(6U, line);
    }
    (void)snprintf(line, sizeof(line), "N=%lu Z=%u.%02u", (unsigned long)phase3_event_count, z/100U, z%100U);
    BSP_LCD_SetLine(7U, line);
    (void)snprintf(line, sizeof(line), "A=%u I=%u E=%u ZR=%u", (unsigned)phase3_adc.valid,
                   (unsigned)phase3_capture.valid, (unsigned)phase3_start_errors, (unsigned)phase3_zero_result);
    BSP_LCD_SetLine(8U, line);
}

void App_Init(void)
{
    uint32_t now;
    BSP_LED_Init();
    BSP_LCD_Init();
    BSP_LCD_SetLine(1U, "    PHASE3 CHECK");
    BSP_LCD_SetLine(9U, " TEST ONLY +/- PWM");
    if (SystemCoreClock != 80000000U) Error_Handler();
    phase3_start_errors = 0U;
    if (!BSP_ADC_Init()) phase3_start_errors |= 1U;
    if (!BSP_Capture_Init()) phase3_start_errors |= 2U;
    if (!BSP_PWM_Start()) phase3_start_errors |= 4U;
    requested_duty = 500U;
    phase3_offset = phase3_pressure = phase3_flow = 0.0f;
    phase3_event_count = phase3_last_held_ms = phase3_max_loop_gap_ms = 0U;
    phase3_last_key = phase3_last_type = phase3_zero_result = 0U;
    now = HAL_GetTick();
    previous_ms = now;
    BSP_Keys_Init(&keys, now);
    BSP_Periodic_Init(&key_timer, now);
    BSP_Periodic_Init(&sample_timer, now);
    BSP_Periodic_Init(&screen_timer, now);
}

void App_Run(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t gap = (uint32_t)(now - previous_ms);
    BSP_KeyEvent events[BSP_KEY_COUNT];
    uint8_t i, count;
    previous_ms = now;
    if (gap > phase3_max_loop_gap_ms) phase3_max_loop_gap_ms = gap;

    if (BSP_Periodic_Due(&key_timer, now, 5U)) {
        count = BSP_Keys_Scan(&keys, HAL_GetTick(), events);
        for (i = 0U; i < count; ++i) HandleEvent(&events[i]);
        BSP_LED_Write(BSP_Keys_PressedMask(&keys)); /* 测试：LD1~4对应稳定按下状态。 */
    }
    if (BSP_Periodic_Due(&sample_timer, now, 10U)) {
        (void)BSP_ADC_Read(&phase3_adc);
        BSP_Capture_Read(&phase3_capture);
    }
    phase3_pressure = phase3_adc.valid ? Measurement_Pressure(phase3_adc.voltage_v, phase3_offset) : 0.0f;
    phase3_flow = phase3_capture.valid ? Measurement_Flow(phase3_capture.frequency_hz) : 0.0f;
    if (BSP_Periodic_Due(&screen_timer, now, 100U)) ComposeScreen();
    BSP_LCD_Service();
}
