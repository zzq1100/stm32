/* 第二阶段板上验证入口，不是竞赛业务 app.c。
 * 使用与最终工程相同的 BSP 模块；最终目标不得链接此文件。 */
#include "app.h"
#include "main.h"
#include "bsp_led.h"
#include "bsp_lcd.h"
#include "bsp_time.h"
#include <stdio.h>

/* volatile 便于在 Keil Watch 中观察；这些是验证计数，不是业务数据。 */
volatile uint32_t phase2_uptime_ms;
volatile uint32_t phase2_slots_5ms;
volatile uint32_t phase2_slots_10ms;
volatile uint32_t phase2_slots_100ms;
volatile uint32_t phase2_max_loop_gap_ms;
volatile uint8_t phase2_led_mask;

static BSP_Periodic timer_5ms, timer_10ms, timer_100ms, timer_led;
static uint32_t started_ms, previous_ms;
static uint8_t led_index;

void App_Init(void)
{
    uint32_t now;

    BSP_LED_Init();                 /* 先关闭锁存器，再初始化共用总线的 LCD。 */
    BSP_LCD_Init();
    BSP_LCD_SetLine(1U, "    PHASE2 CHECK");
    BSP_LCD_SetLine(3U, "   SYS=80MHz");
    BSP_LCD_SetLine(8U, "   TEST ONLY");

    if (SystemCoreClock != 80000000U) {
        Error_Handler();
    }

    /* LCD 启动耗时不计入周期任务，初始化结束后再建立起点。 */
    now = HAL_GetTick();
    started_ms = previous_ms = now;
    BSP_Periodic_Init(&timer_5ms, now);
    BSP_Periodic_Init(&timer_10ms, now);
    BSP_Periodic_Init(&timer_100ms, now);
    BSP_Periodic_Init(&timer_led, now);
    phase2_uptime_ms = phase2_slots_5ms = phase2_slots_10ms = 0U;
    phase2_slots_100ms = phase2_max_loop_gap_ms = 0U;
    led_index = 0U;
    phase2_led_mask = 1U;
    BSP_LED_Write(phase2_led_mask);
}

void App_Run(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t gap = (uint32_t)(now - previous_ms);
    char line[21];

    previous_ms = now;
    phase2_uptime_ms = (uint32_t)(now - started_ms);
    if (gap > phase2_max_loop_gap_ms) {
        phase2_max_loop_gap_ms = gap;
    }

    if (BSP_Periodic_Due(&timer_5ms, now, 5U)) {
        ++phase2_slots_5ms;
    }
    if (BSP_Periodic_Due(&timer_10ms, now, 10U)) {
        ++phase2_slots_10ms;
    }
    if (BSP_Periodic_Due(&timer_100ms, now, 100U)) {
        ++phase2_slots_100ms;
        (void)snprintf(line, sizeof(line), "   T=%lu.%lus",
                       (unsigned long)(phase2_uptime_ms / 1000U),
                       (unsigned long)((phase2_uptime_ms % 1000U) / 100U));
        BSP_LCD_SetLine(4U, line);
        (void)snprintf(line, sizeof(line), "   LD=%u", (unsigned)(led_index + 1U));
        BSP_LCD_SetLine(5U, line);
        (void)snprintf(line, sizeof(line), "   GAP=%lums",
                       (unsigned long)phase2_max_loop_gap_ms);
        BSP_LCD_SetLine(6U, line);
    }
    if (BSP_Periodic_Due(&timer_led, now, 1000U)) {
        led_index = (uint8_t)((led_index + 1U) % 4U);
        phase2_led_mask = (uint8_t)(1U << led_index);
        BSP_LED_Write(phase2_led_mask); /* 只轮流点亮 LD1~LD4，LD5~LD8 始终灭。 */
    }

    BSP_LCD_Service();             /* 每轮都给显示机会，不等到100ms才画一个字。 */
}
