#include "monitor.h"
#include <string.h>

void Monitor_Init(Monitor *m)
{
    memset(m, 0, sizeof(*m));
    m->active.target_d = 5; m->active.target_p = 50;
    m->active.fh = 200; m->active.fl = 100;
    m->active.pl = 10; m->active.dh = 65;
    m->draft = m->active; m->duty = 500;
}

void Monitor_Measure(Monitor *m, float voltage, float frequency)
{
    m->voltage = voltage;
    m->frequency = frequency;
    m->pressure = voltage <= m->offset ? 0.0f :
                  (voltage - m->offset) * 10.0f / (3.3f - m->offset);
    if (m->pressure > 10.0f) m->pressure = 10.0f;
    m->flow = frequency < 800.0f ? 0.0f :
              (frequency > 8000.0f ? 40.0f : frequency / 200.0f);
}

void Monitor_Integrate100ms(Monitor *m)
{
    uint32_t whole;
    /* F 单位 L/min，每 100 ms 的增量为 F/600 L。
       整数升与小数余量分别保存，避免累计量大时 float 吞掉小增量。 */
    /* 余量以 1/600 L 为单位累加，40 L/min 连续 15 次恰好 1 L，
       避免反复除以 600 的舍入误差使整升更新晚一个采样周期。 */
    m->fraction += (double)m->flow;
    whole = (uint32_t)(m->fraction / 600.0);
    if (whole) {
        m->fraction -= (double)whole * 600.0;
        if (UINT32_MAX - m->litres >= whole) m->litres += whole;
        else { m->litres = UINT32_MAX; m->fraction = 0; }
    }
}

void Monitor_Control10ms(Monitor *m)
{
    int direction = 0;
    if (m->automatic) {
        float target = (float)m->active.target_p / 10.0f;
        if (m->pressure < target - 0.5f) direction = 1;
        else if (m->pressure > target + 0.5f) direction = -1;
        else m->adjusting = 0;
    } else {
        uint16_t target = m->active.target_d * 100U;
        if (m->duty < target) direction = 1;
        else if (m->duty > target) direction = -1;
        else m->adjusting = 0;
    }
    /* 每 10 ms 改变 0.01 个百分点，即每秒 1 个百分点。
       上下限只限制输出；自动模式在限位但压力未到目标时仍未达标。 */
    if (direction > 0 && m->duty < 9500) ++m->duty;
    if (direction < 0 && m->duty > 500) --m->duty;
    if (!m->automatic && m->duty == m->active.target_d * 100U) m->adjusting = 0;
}

static uint8_t Persistent(uint8_t condition, uint8_t *timing,
                          uint32_t *start, uint32_t now)
{
    if (!condition) { *timing = 0; return 0; }
    if (!*timing) { *timing = 1; *start = now; }
    /* 题目为“超过 2 秒”，不是达到 2 秒。无符号差值允许 tick 回绕。 */
    return (uint32_t)(now - *start) > 2000U;
}

void Monitor_Alarms(Monitor *m, uint32_t now)
{
    uint8_t mask = m->frequency > 8000.0f ? 1U : 0U;
    if (Persistent(m->flow < (float)m->active.fl / 10.0f &&
                   m->duty > m->active.dh * 100U,
                   &m->block_timing, &m->block_start, now)) mask |= 2U;
    if (Persistent(m->flow > (float)m->active.fh / 10.0f &&
                   m->pressure < (float)m->active.pl / 10.0f,
                   &m->leak_timing, &m->leak_start, now)) mask |= 4U;
    if (m->adjusting) mask |= 8U;
    m->leds = mask; /* LD5..LD8 始终为零 */
}

static uint16_t Adjust(uint16_t value, int step, uint16_t low, uint16_t high)
{
    int result = (int)value + step;
    if (result < low) result = low;
    if (result > high) result = high;
    return (uint16_t)result;
}

void Monitor_Key(Monitor *m, uint8_t key, uint8_t down,
                 uint32_t held_ms, MonitorPage origin_page)
{
    int sign;
    if (!down) {
        if (origin_page != PAGE_MAIN || m->page != PAGE_MAIN || held_ms < 2000U) return;
        if (key == 3) {
            /* 满量程时无法构造有限斜率，保留原零点；其余电压即时校准。 */
            if (m->voltage < 3.3f) {
                m->offset = m->voltage;
                Monitor_Measure(m, m->voltage, m->frequency);
            }
        } else if (key == 4) {
            m->litres = 0; m->fraction = 0;
        }
        return;
    }
    if (key == 1) {
        /* 只提交正在退出的设置页，确保另一个设置页的数据不会被覆盖。 */
        if (m->page == PAGE_OUTP) {
            if ((!m->automatic && m->active.target_d != m->draft.target_d) ||
                (m->automatic && m->active.target_p != m->draft.target_p)) m->adjusting = 1;
            m->active.target_d = m->draft.target_d;
            m->active.target_p = m->draft.target_p;
        } else if (m->page == PAGE_PARA) {
            m->active.fh = m->draft.fh; m->active.fl = m->draft.fl;
            m->active.pl = m->draft.pl; m->active.dh = m->draft.dh;
        }
        m->page = (MonitorPage)((m->page + 1) % 3);
        m->selection = 0; m->draft = m->active;
    } else if (key == 2) {
        if (m->page == PAGE_MAIN) {
            m->automatic ^= 1U;
            m->adjusting = 1; /* 新模式开始跟踪其当前生效目标 */
        } else m->selection = (uint8_t)((m->selection + 1) % (m->page == PAGE_OUTP ? 2 : 4));
    } else if ((key == 3 || key == 4) && m->page != PAGE_MAIN) {
        sign = key == 3 ? 1 : -1;
        if (m->page == PAGE_OUTP) {
            if (m->selection == 0) m->draft.target_d = Adjust(m->draft.target_d, sign*5, 5, 95);
            else m->draft.target_p = Adjust(m->draft.target_p, sign*5, 10, 95);
        } else {
            switch (m->selection) {
            case 0: m->draft.fh = Adjust(m->draft.fh, sign*10, 40, 400); break;
            case 1: m->draft.fl = Adjust(m->draft.fl, sign*10, 40, 400); break;
            case 2: m->draft.pl = Adjust(m->draft.pl, sign*5, 10, 95); break;
            default: m->draft.dh = Adjust(m->draft.dh, sign*10, 5, 95); break;
            }
        }
    }
}
