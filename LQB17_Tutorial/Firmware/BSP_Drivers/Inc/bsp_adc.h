#ifndef BSP_ADC_H
#define BSP_ADC_H

#include <stdint.h>

typedef struct {
    uint16_t raw;                  /* 12位原始码0~4095，供定位采集问题。 */
    float voltage_v;               /* 按题目3.3V参考换算，单位V。 */
    uint8_t valid;                 /* 本次转换完整成功才为1。 */
} BSP_ADC_Sample;

/* MX_ADC2_Init后、第一次转换前，执行单端硬件校准；成功返回1。 */
uint8_t BSP_ADC_Init(void);

/* 一次软件触发转换，短轮询；成功返回1。
 * 失败清空sample并令valid=0，不能将旧电压当作本次有效数据。
 * 仅在主循环调用；内部轮询的超时参数为1ms，不是无限等待。 */
uint8_t BSP_ADC_Read(BSP_ADC_Sample *sample);

#endif
