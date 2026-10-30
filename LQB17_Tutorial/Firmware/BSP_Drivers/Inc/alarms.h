#ifndef ALARMS_H
#define ALARMS_H
#include "monitor_types.h"
typedef struct {
    uint32_t since_ms;
    uint8_t active, asserted;     /* active记录起点有效；asserted保持已确认报警。 */
} AlarmDelay;
typedef struct {
    AlarmDelay blockage, leakage;
    SafetyConfig previous;
    uint8_t have_parameters;
} Alarms;
void Alarms_Init(Alarms *a);
/* 返回逻辑灯掩码：bit0~3对应LD1~4；高四位始终0。 */
uint8_t Alarms_Update(Alarms *a, const SafetyConfig *cfg,
                      const MonitorMeasurements *m, uint16_t actual_duty,
                      uint8_t tracking, uint32_t now_ms);
#endif
