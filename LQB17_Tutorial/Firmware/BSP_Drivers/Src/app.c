/* 最终竞赛入口；Final目标排除Tests中的两个阶段App，避免重复定义。 */
#include "app.h"
#include "main.h"
#include "bsp_led.h"
#include "bsp_lcd.h"
#include "bsp_time.h"
#include "bsp_keys.h"
#include "bsp_adc.h"
#include "bsp_capture.h"
#include "bsp_pwm.h"
#include "measurement.h"
#include "settings.h"
#include "control.h"
#include "totalizer.h"
#include "alarms.h"
#include "display.h"
#include <string.h>

/* 全局对象便于Keil Watch观察；只有App负责调度和跨模块连接。 */
Settings app_settings;
Control app_control;
Totalizer app_total;
Alarms app_alarms;
MonitorMeasurements app_measurements;
BSP_ADC_Sample app_adc;
BSP_CaptureSample app_capture;
volatile uint32_t app_max_loop_gap_ms, app_missed_slots;
volatile uint8_t app_start_errors, app_zero_result, app_led_mask;
static BSP_Keys keys;
static BSP_Periodic key_timer, sample_timer, control_timer, total_timer, screen_timer;
static uint32_t previous_ms;

static uint8_t Due(BSP_Periodic *timer, uint32_t now, uint32_t period)
{
    uint32_t missed;
    if (!BSP_Periodic_Due(timer, now, period)) return 0U;
    missed = (uint32_t)(now - timer->last_ms) / period;
    if (missed) {
        /* 不用当前样本重复“补造”过去的采样。漏槽可在Watch中发现。 */
        timer->last_ms += missed * period;
        if (UINT32_MAX - app_missed_slots < missed) app_missed_slots = UINT32_MAX;
        else app_missed_slots += missed;
    }
    return 1U;
}
static void ReadPressure(void)
{
    (void)BSP_ADC_Read(&app_adc);
    app_measurements.pressure_valid = app_adc.valid;
    app_measurements.voltage_v = app_adc.voltage_v;
    app_measurements.pressure_bar = app_adc.valid ?
        Measurement_Pressure(app_adc.voltage_v, app_measurements.offset_v) : 0.0f;
}
static void ReadFlow(void)
{
    BSP_Capture_Read(&app_capture);
    app_measurements.capture_valid = app_capture.valid;
    app_measurements.frequency_hz = app_capture.frequency_hz;
    app_measurements.flow_lpm = app_capture.valid ? Measurement_Flow(app_capture.frequency_hz) : 0.0f;
}
static uint8_t HandleEvent(const BSP_KeyEvent *event)
{
    uint8_t requests = Settings_Handle(&app_settings, event);
    if (requests & SETTINGS_ZERO_REQUEST) {
        ReadPressure();           /* 校准取松开处理时的新样本，而不是按下时的旧值。 */
        if (app_adc.valid) {
            app_zero_result = Measurement_Calibrate(app_adc.voltage_v, &app_measurements.offset_v) ? 1U : 2U;
            app_measurements.pressure_bar = Measurement_Pressure(app_adc.voltage_v, app_measurements.offset_v);
        } else app_zero_result = 3U;
    }
    if (requests & SETTINGS_CLEAR_REQUEST) {
        Totalizer_Clear(&app_total);
        BSP_Periodic_Init(&total_timer, HAL_GetTick()); /* 清零后重新开始完整100ms积分区间。 */
    }
    if (requests & (SETTINGS_MODE_CHANGED | SETTINGS_OUTPUT_CHANGED)) {
        if (Control_Configure(&app_control, app_settings.mode, &app_settings.output,
                              app_measurements.pressure_bar, app_measurements.pressure_valid)) {
            BSP_Periodic_Init(&control_timer, HAL_GetTick());
        }
    }
    return (uint8_t)(requests != 0U || event->type == BSP_KEY_PRESS);
}
void App_Init(void)
{
    uint32_t now;
    BSP_LED_Init();
    BSP_LCD_Init();
    Settings_Init(&app_settings);
    Control_Init(&app_control);
    Totalizer_Clear(&app_total);
    Alarms_Init(&app_alarms);
    memset(&app_measurements, 0, sizeof(app_measurements));
    memset(&app_adc, 0, sizeof(app_adc));
    memset(&app_capture, 0, sizeof(app_capture));
    app_max_loop_gap_ms = app_missed_slots = 0U;
    app_start_errors = app_zero_result = app_led_mask = 0U;
    if (SystemCoreClock != 80000000U) app_start_errors |= 8U;
    if (!BSP_ADC_Init()) app_start_errors |= 1U;
    if (!BSP_Capture_Init()) app_start_errors |= 2U;
    if (!BSP_PWM_Start()) app_start_errors |= 4U;
    if (app_start_errors) { Error_Handler(); return; }
    now = HAL_GetTick();
    previous_ms = now;
    BSP_Keys_Init(&keys, now);
    BSP_Periodic_Init(&key_timer, now);
    BSP_Periodic_Init(&sample_timer, now);
    BSP_Periodic_Init(&control_timer, now);
    BSP_Periodic_Init(&total_timer, now);
    BSP_Periodic_Init(&screen_timer, now);
    Display_Render(&app_settings, &app_measurements, BSP_PWM_GetDuty(), app_total.litres);
}
void App_Run(void)
{
    uint32_t now = HAL_GetTick(), gap = (uint32_t)(now - previous_ms);
    BSP_KeyEvent events[BSP_KEY_COUNT];
    uint8_t count, i, redraw = 0U;
    if (app_start_errors) return;
    previous_ms = now;
    if (gap > app_max_loop_gap_ms) app_max_loop_gap_ms = gap;
    if (Due(&sample_timer, now, 10U)) { ReadPressure(); ReadFlow(); }
    if (Due(&key_timer, now, 5U)) {
        count = BSP_Keys_Scan(&keys, HAL_GetTick(), events);
        for (i = 0U; i < count; ++i) redraw |= HandleEvent(&events[i]);
    }
    /* 事件处理可能重设控制/积分计时器；重新取now，避免旧now减新起点。 */
    now = HAL_GetTick();
    if (Due(&control_timer, now, 10U)) {
        Control_Step10ms(&app_control, app_measurements.pressure_bar, app_measurements.pressure_valid);
        BSP_PWM_SetDuty(app_control.duty_centi_pct);
    }
    if (Due(&total_timer, now, 100U)) Totalizer_Sample100ms(&app_total, app_measurements.flow_lpm);
    app_led_mask = Alarms_Update(&app_alarms, &app_settings.safety, &app_measurements,
                                 BSP_PWM_GetDuty(), app_control.tracking, now);
    BSP_LED_Write(app_led_mask);
    if (Due(&screen_timer, now, 100U) || redraw)
        Display_Render(&app_settings, &app_measurements, BSP_PWM_GetDuty(), app_total.litres);
    BSP_LCD_Service();            /* 每轮最多一个变化字符；共用总线只在主循环访问。 */
}
