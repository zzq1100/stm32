#ifndef CONTROL_H
#define CONTROL_H
#include "monitor_types.h"
typedef struct {
    MonitorMode mode;
    OutputConfig target;
    uint16_t duty_centi_pct;      /* 内部细分请求，500=5.00%，不从CCR反复回算。 */
    uint8_t tracking;             /* 当前目标提交/切模式后，尚未达标的一次过程。 */
} Control;
void Control_Init(Control *c);
/* 返回1：当前模式或相关目标改变，App重设控制节拍起点。 */
uint8_t Control_Configure(Control *c, MonitorMode mode, const OutputConfig *target,
                          float pressure_bar, uint8_t pressure_valid);
/* 每个实际10ms控制槽调用一次；不等待，不操作外设。 */
void Control_Step10ms(Control *c, float pressure_bar, uint8_t pressure_valid);
#endif
