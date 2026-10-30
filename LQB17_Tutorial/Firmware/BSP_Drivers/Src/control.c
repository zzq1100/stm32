#include "control.h"

static uint8_t ValidPressure(float p, uint8_t valid)
{
    return (uint8_t)(valid && p >= 0.0f && p <= 10.0f);
}
static uint8_t Reached(const Control *c, float pressure, uint8_t valid)
{
    float target;
    if (c->mode == MONITOR_MAN)
        return (uint8_t)(c->duty_centi_pct == c->target.tar_d_pct * 100U);
    target = c->target.tar_p_deci / 10.0f;
    return (uint8_t)(ValidPressure(pressure, valid) &&
                     pressure >= target - 0.5f && pressure <= target + 0.5f);
}
void Control_Init(Control *c)
{
    c->mode = MONITOR_MAN;
    c->target.tar_d_pct = 5U;
    c->target.tar_p_deci = 50U;
    c->duty_centi_pct = 500U;
    c->tracking = 0U;
}
uint8_t Control_Configure(Control *c, MonitorMode mode, const OutputConfig *target,
                          float pressure_bar, uint8_t pressure_valid)
{
    uint8_t changed = (uint8_t)(mode != c->mode ||
        (mode == MONITOR_MAN && target->tar_d_pct != c->target.tar_d_pct) ||
        (mode == MONITOR_AUTO && target->tar_p_deci != c->target.tar_p_deci));
    c->mode = mode;
    c->target = *target;
    if (changed) c->tracking = (uint8_t)!Reached(c, pressure_bar, pressure_valid);
    return changed;
}
void Control_Step10ms(Control *c, float pressure_bar, uint8_t pressure_valid)
{
    uint16_t target_d;
    float target_p;
    if (c->mode == MONITOR_MAN) {
        target_d = (uint16_t)(c->target.tar_d_pct * 100U);
        if (c->duty_centi_pct < target_d) ++c->duty_centi_pct;
        else if (c->duty_centi_pct > target_d) --c->duty_centi_pct;
    } else if (ValidPressure(pressure_bar, pressure_valid)) {
        target_p = c->target.tar_p_deci / 10.0f;
        if (pressure_bar < target_p - 0.5f && c->duty_centi_pct < 9500U) ++c->duty_centi_pct;
        else if (pressure_bar > target_p + 0.5f && c->duty_centi_pct > 500U) --c->duty_centi_pct;
    }
    /* 防御性限幅；正常设置输入本就处于题目范围。 */
    if (c->duty_centi_pct < 500U) c->duty_centi_pct = 500U;
    if (c->duty_centi_pct > 9500U) c->duty_centi_pct = 9500U;
    if (c->tracking && Reached(c, pressure_bar, pressure_valid)) c->tracking = 0U;
    /* 达标后受扰，控制仍工作，但不自动重新点亮一次性调节指示。 */
}
