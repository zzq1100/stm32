#include "app.h"
#include "bsp_board.h"
#include "bsp_display.h"
#include "bsp_keys.h"
#include "main.h"

static Monitor monitor;
static Keys keys;
static uint32_t key_tick, control_tick, sample_tick;

void App_Init(void)
{
    BSP_BoardInit();
    Monitor_Init(&monitor);
    Keys_Init(&keys);
    Display_Init();
    BSP_SetLEDs(0);
    key_tick = control_tick = sample_tick = HAL_GetTick();
    Display_Compose(&monitor);
}

void App_Run(void)
{
    uint32_t now = HAL_GetTick();
    float voltage;
    if ((uint32_t)(now - control_tick) >= 10U) {
        control_tick += 10U;
        voltage = monitor.voltage;
        /* 采样失败保留上次有效电压；输入无脉冲则由驱动超时返回 0 Hz。 */
        BSP_ReadVoltage(&voltage);
        Monitor_Measure(&monitor, voltage, BSP_ReadFrequency(now));
        Monitor_Control10ms(&monitor);
        BSP_SetDuty(monitor.duty);
    }
    if ((uint32_t)(now - key_tick) >= 5U) {
        MonitorPage old_page = monitor.page;
        uint8_t old_mode = monitor.automatic;
        key_tick = now;
        Keys_Scan(&keys, &monitor, BSP_ReadKeys(), now);
        /* 切页/切模式立即更新目标画面；数值按 100 ms 周期更新。 */
        if (old_page != monitor.page || old_mode != monitor.automatic) Display_Compose(&monitor);
        else if (monitor.page != PAGE_MAIN) Display_Compose(&monitor);
    }
    if ((uint32_t)(now - sample_tick) >= 100U) {
        sample_tick += 100U;
        Monitor_Integrate100ms(&monitor);
        Display_Compose(&monitor);
    }
    Monitor_Alarms(&monitor, now);
    BSP_SetLEDs(monitor.leds);
    Display_Service();
}
