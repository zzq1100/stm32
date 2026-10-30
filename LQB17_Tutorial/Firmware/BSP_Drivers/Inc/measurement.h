#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <stdint.h>

/* 纯计算模块，无HAL依赖。只有有效采样才能传入参与控制。 */
float Measurement_Pressure(float voltage_v, float offset_v);
/* 有效范围0<=voltage<3.3V；拒绝满量程/非法值并保持旧零点。 */
uint8_t Measurement_Calibrate(float voltage_v, float *offset_v);
/* P02-18~20：800~8000Hz含端点；超范围按题目处理。 */
float Measurement_Flow(float frequency_hz);
uint8_t Measurement_Overrange(float frequency_hz);

#endif
