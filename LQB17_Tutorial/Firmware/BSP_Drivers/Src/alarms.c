#include "alarms.h"
#include "measurement.h"
#include <string.h>

static uint8_t Delayed(AlarmDelay *d, uint8_t condition, uint32_t now)
{
    if (!condition) { d->active = d->asserted = 0U; return 0U; }
    if (!d->active) { d->active = 1U; d->asserted = 0U; d->since_ms = now; }
    /* 原文是“超过2秒”，恰好2000ms还不亮。无符号差值允许tick回绕。 */
    if ((uint32_t)(now - d->since_ms) > 2000U) d->asserted = 1U;
    return d->asserted;            /* 持续超过一次tick回绕也不重新熄灭。 */
}
void Alarms_Init(Alarms *a)
{
    memset(a, 0, sizeof(*a));
}
uint8_t Alarms_Update(Alarms *a, const SafetyConfig *cfg,
                      const MonitorMeasurements *m, uint16_t actual_duty,
                      uint8_t tracking, uint32_t now_ms)
{
    uint8_t mask = 0U, blocked, leaked;
    /* 新阈值生效后重新观察相关条件；改FH/PL不打断堵塞计时。 */
    if (a->have_parameters) {
        if (cfg->fl_deci != a->previous.fl_deci || cfg->dh_pct != a->previous.dh_pct)
            a->blockage.active = 0U;
        if (cfg->fh_deci != a->previous.fh_deci || cfg->pl_deci != a->previous.pl_deci)
            a->leakage.active = 0U;
    }
    a->previous = *cfg;
    a->have_parameters = 1U;
    if (m->capture_valid && Measurement_Overrange(m->frequency_hz)) mask |= 1U;
    blocked = (uint8_t)(m->flow_lpm < cfg->fl_deci / 10.0f &&
                        actual_duty > cfg->dh_pct * 100U);
    leaked = (uint8_t)(m->pressure_valid && m->flow_lpm > cfg->fh_deci / 10.0f &&
                       m->pressure_bar < cfg->pl_deci / 10.0f);
    if (Delayed(&a->blockage, blocked, now_ms)) mask |= 2U;
    if (Delayed(&a->leakage, leaked, now_ms)) mask |= 4U;
    if (tracking) mask |= 8U;
    return mask;
}
