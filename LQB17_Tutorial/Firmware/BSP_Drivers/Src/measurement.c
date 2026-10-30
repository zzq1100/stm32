#include "measurement.h"
#include <float.h>

static uint8_t NonnegativeFinite(float value)
{
    /* NaN与任意数比较均为假；无穷大超过FLT_MAX。 */
    return (uint8_t)(value >= 0.0f && value <= FLT_MAX);
}

float Measurement_Pressure(float voltage_v, float offset_v)
{
    if (!NonnegativeFinite(voltage_v) || !(offset_v >= 0.0f && offset_v < 3.3f)) return 0.0f;
    if (voltage_v <= offset_v) return 0.0f;
    if (voltage_v >= 3.3f) return 10.0f;
    /* P02-14~16：校准后分母也改变，满量程仍对应10BAR。 */
    return (voltage_v - offset_v) * 10.0f / (3.3f - offset_v);
}

uint8_t Measurement_Calibrate(float voltage_v, float *offset_v)
{
    if (offset_v == 0 || !(voltage_v >= 0.0f && voltage_v < 3.3f)) return 0U;
    *offset_v = voltage_v;
    return 1U;
}

float Measurement_Flow(float frequency_hz)
{
    if (!NonnegativeFinite(frequency_hz) || frequency_hz < 800.0f) return 0.0f;
    if (frequency_hz > 8000.0f) return 40.0f;
    return frequency_hz / 200.0f;
}

uint8_t Measurement_Overrange(float frequency_hz)
{
    /* 保留原始频率判断超频，不能从已经限幅的流量40反推。 */
    return (uint8_t)(NonnegativeFinite(frequency_hz) && frequency_hz > 8000.0f);
}
