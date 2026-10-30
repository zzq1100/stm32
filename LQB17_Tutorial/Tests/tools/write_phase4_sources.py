from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
FW = ROOT / 'Firmware/BSP_Drivers'
def put(rel, text):
    p = FW / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text.strip()+'\n', encoding='utf-8')

put('Inc/monitor_types.h', r'''
#ifndef MONITOR_TYPES_H
#define MONITOR_TYPES_H
#include <stdint.h>

/* 公共类型先约定单位；目标整数%与实际输出0.01%不能混用。 */
typedef enum { MONITOR_MAIN, MONITOR_OUTP, MONITOR_PARA } MonitorPage;
typedef enum { MONITOR_MAN, MONITOR_AUTO } MonitorMode;
typedef struct {
    uint16_t tar_d_pct;           /* TarD：整数百分比，5~95，默认5。 */
    uint16_t tar_p_deci;          /* TarP：0.1BAR，10~95，默认50。 */
} OutputConfig;
typedef struct {
    uint16_t fh_deci, fl_deci;    /* FH/FL：0.1L/min，40~400。 */
    uint16_t pl_deci;             /* PL：0.1BAR，10~95。 */
    uint16_t dh_pct;              /* DH：整数百分比，5~95。 */
} SafetyConfig;
typedef struct {
    float voltage_v, offset_v, pressure_bar;
    float frequency_hz, flow_lpm;
    uint8_t pressure_valid;       /* ADC失败不能当作真实低压力控制。 */
    uint8_t capture_valid;        /* 无信号/无有效周期时F按0处理。 */
} MonitorMeasurements;
#endif
''')
put('Inc/settings.h', r'''
#ifndef SETTINGS_H
#define SETTINGS_H
#include "monitor_types.h"
#include "bsp_keys.h"

#define SETTINGS_MODE_CHANGED   0x01U
#define SETTINGS_OUTPUT_CHANGED 0x02U
#define SETTINGS_PARAMS_CHANGED 0x04U
#define SETTINGS_ZERO_REQUEST   0x08U
#define SETTINGS_CLEAR_REQUEST  0x10U
#define SETTINGS_PAGE_CHANGED   0x20U

typedef struct {
    MonitorPage page;
    MonitorMode mode;
    OutputConfig output, output_draft; /* 生效值/编辑副本；控制只读output。 */
    SafetyConfig safety, safety_draft;
    uint8_t selected;             /* OUTP 0~1；PARA 0~3；进入时回到0。 */
    uint8_t long_eligible;        /* bit2/bit3：本次B3/B4是否在MAIN按下。 */
} Settings;

void Settings_Init(Settings *s);
/* 每个防抖事件调用一次，返回操作请求位；不直接访问ADC或PWM。 */
uint8_t Settings_Handle(Settings *s, const BSP_KeyEvent *event);
#endif
''')
put('Src/settings.c', r'''
#include "settings.h"
#include <string.h>

static uint16_t Adjust(uint16_t value, int delta, uint16_t low, uint16_t high)
{
    /* 先用有符号数计算，避免5-10在无符号类型中绕成很大的数。 */
    int32_t next = (int32_t)value + delta;
    if (next < low) next = low;
    if (next > high) next = high;
    return (uint16_t)next;
}

void Settings_Init(Settings *s)
{
    memset(s, 0, sizeof(*s));
    s->page = MONITOR_MAIN;
    s->mode = MONITOR_MAN;
    s->output.tar_d_pct = 5U;
    s->output.tar_p_deci = 50U;
    s->safety.fh_deci = 200U;
    s->safety.fl_deci = 100U;
    s->safety.pl_deci = 10U;
    s->safety.dh_pct = 65U;
    s->output_draft = s->output;
    s->safety_draft = s->safety;
}

static uint8_t NextPage(Settings *s)
{
    uint8_t result = SETTINGS_PAGE_CHANGED;
    s->long_eligible = 0U;        /* P05-09/10：切过页，原长按资格永久取消。 */
    s->selected = 0U;
    if (s->page == MONITOR_MAIN) {
        s->output_draft = s->output;
        s->page = MONITOR_OUTP;
    } else if (s->page == MONITOR_OUTP) {
        if (s->output.tar_d_pct != s->output_draft.tar_d_pct ||
            s->output.tar_p_deci != s->output_draft.tar_p_deci) {
            result |= SETTINGS_OUTPUT_CHANGED;
        }
        s->output = s->output_draft; /* 退出OUTP才提交这一页的两个目标。 */
        s->safety_draft = s->safety;
        s->page = MONITOR_PARA;
    } else {
        if (s->safety.fh_deci != s->safety_draft.fh_deci ||
            s->safety.fl_deci != s->safety_draft.fl_deci ||
            s->safety.pl_deci != s->safety_draft.pl_deci ||
            s->safety.dh_pct != s->safety_draft.dh_pct) {
            result |= SETTINGS_PARAMS_CHANGED;
        }
        s->safety = s->safety_draft;
        s->page = MONITOR_MAIN;
    }
    return result;
}

static void EditSelected(Settings *s, int direction)
{
    if (s->page == MONITOR_OUTP) {
        if (s->selected == 0U)
            s->output_draft.tar_d_pct = Adjust(s->output_draft.tar_d_pct, direction*5, 5U, 95U);
        else
            s->output_draft.tar_p_deci = Adjust(s->output_draft.tar_p_deci, direction*5, 10U, 95U);
    } else if (s->page == MONITOR_PARA) {
        switch (s->selected) {
        case 0U: s->safety_draft.fh_deci = Adjust(s->safety_draft.fh_deci, direction*10, 40U, 400U); break;
        case 1U: s->safety_draft.fl_deci = Adjust(s->safety_draft.fl_deci, direction*10, 40U, 400U); break;
        case 2U: s->safety_draft.pl_deci = Adjust(s->safety_draft.pl_deci, direction*5, 10U, 95U); break;
        case 3U: s->safety_draft.dh_pct = Adjust(s->safety_draft.dh_pct, direction*10, 5U, 95U); break;
        default: break;
        }
    }
}

uint8_t Settings_Handle(Settings *s, const BSP_KeyEvent *event)
{
    uint8_t bit, eligible;
    if (event->key < 1U || event->key > 4U) return 0U;
    bit = (uint8_t)(1U << (event->key - 1U));
    if (event->type == BSP_KEY_RELEASE) {
        eligible = (uint8_t)(s->long_eligible & bit);
        s->long_eligible &= (uint8_t)~bit;
        if (s->page == MONITOR_MAIN && eligible && event->held_ms >= 2000U) {
            if (event->key == 3U) return SETTINGS_ZERO_REQUEST;
            if (event->key == 4U) return SETTINGS_CLEAR_REQUEST;
        }
        return 0U;
    }
    if (event->type != BSP_KEY_PRESS) return 0U;
    if (event->key == 1U) return NextPage(s);
    if (event->key == 2U) {
        if (s->page == MONITOR_MAIN) {
            s->mode = (s->mode == MONITOR_MAN) ? MONITOR_AUTO : MONITOR_MAN;
            return SETTINGS_MODE_CHANGED;
        }
        s->selected = (uint8_t)((s->selected + 1U) % (s->page == MONITOR_OUTP ? 2U : 4U));
    } else if (s->page == MONITOR_MAIN) {
        s->long_eligible |= bit;  /* 只记录资格；按住达到2秒时不执行功能。 */
    } else {
        EditSelected(s, event->key == 3U ? 1 : -1);
    }
    return 0U;
}
''')
put('Inc/control.h', r'''
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
''')
put('Src/control.c', r'''
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
''')
put('Inc/totalizer.h', r'''
#ifndef TOTALIZER_H
#define TOTALIZER_H
#include <stdint.h>
#define TOTALIZER_UNITS_PER_LITRE 600000000UL

typedef struct {
    uint32_t litres;              /* 整升，不与小增量一起保存在float中。 */
    uint32_t remainder;           /* 余量单位1/600000000 L，始终不足一升。 */
} Totalizer;
void Totalizer_Clear(Totalizer *q);
/* 每100ms一个真实采样槽；输入L/min，>40限幅，负值/NaN忽略。 */
void Totalizer_Sample100ms(Totalizer *q, float flow_lpm);
#endif
''')
put('Src/totalizer.c', r'''
#include "totalizer.h"
#include <float.h>

void Totalizer_Clear(Totalizer *q)
{
    q->litres = 0U;
    q->remainder = 0U;            /* 清零必须同时清掉不足一升的余量。 */
}
void Totalizer_Sample100ms(Totalizer *q, float flow_lpm)
{
    uint32_t addition;
    if (!(flow_lpm >= 0.0f && flow_lpm <= FLT_MAX)) return;
    if (q->litres == UINT32_MAX) { q->remainder = 0U; return; }
    if (flow_lpm > 40.0f) flow_lpm = 40.0f;
    /* ΔQ=F/600 L。F先量化到百万分之一L/min，保留小余量，
     * 因此600000000个内部单位才进1L。double仅用于本次有界换算。 */
    addition = (uint32_t)((double)flow_lpm * 1000000.0 + 0.5);
    q->remainder += addition;    /* <600000000+40000000，不会uint32溢出。 */
    if (q->remainder >= TOTALIZER_UNITS_PER_LITRE) {
        q->remainder -= TOTALIZER_UNITS_PER_LITRE;
        ++q->litres;              /* 单次增量<=1/15L，只需进一升。 */
        if (q->litres == UINT32_MAX) q->remainder = 0U;
    }
}
''')
put('Inc/alarms.h', r'''
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
''')
put('Src/alarms.c', r'''
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
''')
put('Inc/display.h', r'''
#ifndef DISPLAY_H
#define DISPLAY_H
#include "settings.h"
#include "bsp_lcd.h"
typedef struct {
    char line[BSP_LCD_ROWS][BSP_LCD_COLS + 1U];
} MonitorScreen;
/* Build只构造文字，可在PC核对行列；Render交给同一份LCD字符服务。 */
void Display_Build(MonitorScreen *screen, const Settings *s,
                   const MonitorMeasurements *m, uint16_t actual_duty, uint32_t litres);
void Display_Render(const Settings *s, const MonitorMeasurements *m,
                    uint16_t actual_duty, uint32_t litres);
#endif
''')
put('Src/display.c', r'''
#include "display.h"
#include <stdio.h>
#include <string.h>

static unsigned Tenth(float value, float maximum)
{
    if (!(value >= 0.0f)) return 0U;
    if (value > maximum) value = maximum;
    return (unsigned)(value * 10.0f + 0.5f); /* 只在显示边界舍入，不改控制原值。 */
}
void Display_Build(MonitorScreen *screen, const Settings *s,
                   const MonitorMeasurements *m, uint16_t actual_duty, uint32_t litres)
{
    unsigned p = Tenth(m->pressure_bar, 10.0f);
    unsigned f = Tenth(m->flow_lpm, 40.0f);
    unsigned v = Tenth(m->offset_v, 3.3f);
    unsigned row, length;
    memset(screen, 0, sizeof(*screen));
    /* 题图1起算标题(2,8)，数据(4,4)；数组下标必须减1。 */
    if (s->page == MONITOR_MAIN) {
        (void)snprintf(screen->line[1], 21U, "       MAIN");
        (void)snprintf(screen->line[3], 21U, "   M=%s", s->mode == MONITOR_AUTO ? "AUTO" : "MAN");
        (void)snprintf(screen->line[4], 21U, "   P=%u.%uBAR", p/10U, p%10U);
        (void)snprintf(screen->line[5], 21U, "   F=%u.%uL/M", f/10U, f%10U);
        (void)snprintf(screen->line[6], 21U, "   Q=%luL", (unsigned long)litres);
        (void)snprintf(screen->line[7], 21U, "   D=%u%%", (unsigned)((actual_duty + 50U)/100U));
        (void)snprintf(screen->line[8], 21U, "   V=%u.%uV", v/10U, v%10U);
    } else if (s->page == MONITOR_OUTP) {
        (void)snprintf(screen->line[1], 21U, "       OUTP");
        (void)snprintf(screen->line[3], 21U, "   TarD=%u%%", (unsigned)s->output_draft.tar_d_pct);
        (void)snprintf(screen->line[4], 21U, "   TarP=%u.%uBAR", (unsigned)(s->output_draft.tar_p_deci/10U), (unsigned)(s->output_draft.tar_p_deci%10U));
    } else {
        (void)snprintf(screen->line[1], 21U, "       PARA");
        (void)snprintf(screen->line[3], 21U, "   FH=%u.%uL/M", (unsigned)(s->safety_draft.fh_deci/10U), (unsigned)(s->safety_draft.fh_deci%10U));
        (void)snprintf(screen->line[4], 21U, "   FL=%u.%uL/M", (unsigned)(s->safety_draft.fl_deci/10U), (unsigned)(s->safety_draft.fl_deci%10U));
        (void)snprintf(screen->line[5], 21U, "   PL=%u.%uBAR", (unsigned)(s->safety_draft.pl_deci/10U), (unsigned)(s->safety_draft.pl_deci%10U));
        (void)snprintf(screen->line[6], 21U, "   DH=%u%%", (unsigned)s->safety_draft.dh_pct);
    }
    for (row = 0U; row < BSP_LCD_ROWS; ++row) {
        length = (unsigned)strlen(screen->line[row]);
        memset(&screen->line[row][length], ' ', BSP_LCD_COLS - length);
        screen->line[row][BSP_LCD_COLS] = '\0'; /* 补空格，切页/数字变短也无残留。 */
    }
}
void Display_Render(const Settings *s, const MonitorMeasurements *m,
                    uint16_t actual_duty, uint32_t litres)
{
    MonitorScreen screen;
    uint8_t row;
    Display_Build(&screen, s, m, actual_duty, litres);
    for (row = 0U; row < BSP_LCD_ROWS; ++row) BSP_LCD_SetLine(row, screen.line[row]);
}
''')
put('Src/app.c', r'''
/* 最终竞赛入口；Final目标排除Tests中的两个阶段App，避免重复定义。 */
#include "app.h"
#include "main.h"
#include "bsp_led.h"
#include "bsp_lcd.h"
#include "bsp_time.h"
#include "bsp_keys.h"
#include "bsp_adc.h"
#include "bsp_capture.h"
#include "bsp_pwm.h"
#include "measurement.h"
#include "settings.h"
#include "control.h"
#include "totalizer.h"
#include "alarms.h"
#include "display.h"
#include <string.h>

/* 全局对象便于Keil Watch观察；只有App负责调度和跨模块连接。 */
Settings app_settings;
Control app_control;
Totalizer app_total;
Alarms app_alarms;
MonitorMeasurements app_measurements;
BSP_ADC_Sample app_adc;
BSP_CaptureSample app_capture;
volatile uint32_t app_max_loop_gap_ms, app_missed_slots;
volatile uint8_t app_start_errors, app_zero_result, app_led_mask;
static BSP_Keys keys;
static BSP_Periodic key_timer, sample_timer, control_timer, total_timer, screen_timer;
static uint32_t previous_ms;

static uint8_t Due(BSP_Periodic *timer, uint32_t now, uint32_t period)
{
    uint32_t missed;
    if (!BSP_Periodic_Due(timer, now, period)) return 0U;
    missed = (uint32_t)(now - timer->last_ms) / period;
    if (missed) {
        /* 不用当前样本重复“补造”过去的采样。漏槽可在Watch中发现。 */
        timer->last_ms += missed * period;
        if (UINT32_MAX - app_missed_slots < missed) app_missed_slots = UINT32_MAX;
        else app_missed_slots += missed;
    }
    return 1U;
}
static void ReadPressure(void)
{
    (void)BSP_ADC_Read(&app_adc);
    app_measurements.pressure_valid = app_adc.valid;
    app_measurements.voltage_v = app_adc.voltage_v;
    app_measurements.pressure_bar = app_adc.valid ?
        Measurement_Pressure(app_adc.voltage_v, app_measurements.offset_v) : 0.0f;
}
static void ReadFlow(void)
{
    BSP_Capture_Read(&app_capture);
    app_measurements.capture_valid = app_capture.valid;
    app_measurements.frequency_hz = app_capture.frequency_hz;
    app_measurements.flow_lpm = app_capture.valid ? Measurement_Flow(app_capture.frequency_hz) : 0.0f;
}
static uint8_t HandleEvent(const BSP_KeyEvent *event)
{
    uint8_t requests = Settings_Handle(&app_settings, event);
    if (requests & SETTINGS_ZERO_REQUEST) {
        ReadPressure();           /* 校准取松开处理时的新样本，而不是按下时的旧值。 */
        if (app_adc.valid) {
            app_zero_result = Measurement_Calibrate(app_adc.voltage_v, &app_measurements.offset_v) ? 1U : 2U;
            app_measurements.pressure_bar = Measurement_Pressure(app_adc.voltage_v, app_measurements.offset_v);
        } else app_zero_result = 3U;
    }
    if (requests & SETTINGS_CLEAR_REQUEST) {
        Totalizer_Clear(&app_total);
        BSP_Periodic_Init(&total_timer, HAL_GetTick()); /* 清零后重新开始完整100ms积分区间。 */
    }
    if (requests & (SETTINGS_MODE_CHANGED | SETTINGS_OUTPUT_CHANGED)) {
        if (Control_Configure(&app_control, app_settings.mode, &app_settings.output,
                              app_measurements.pressure_bar, app_measurements.pressure_valid)) {
            BSP_Periodic_Init(&control_timer, HAL_GetTick());
        }
    }
    return (uint8_t)(requests != 0U || event->type == BSP_KEY_PRESS);
}
void App_Init(void)
{
    uint32_t now;
    BSP_LED_Init();
    BSP_LCD_Init();
    Settings_Init(&app_settings);
    Control_Init(&app_control);
    Totalizer_Clear(&app_total);
    Alarms_Init(&app_alarms);
    memset(&app_measurements, 0, sizeof(app_measurements));
    memset(&app_adc, 0, sizeof(app_adc));
    memset(&app_capture, 0, sizeof(app_capture));
    app_max_loop_gap_ms = app_missed_slots = 0U;
    app_start_errors = app_zero_result = app_led_mask = 0U;
    if (SystemCoreClock != 80000000U) app_start_errors |= 8U;
    if (!BSP_ADC_Init()) app_start_errors |= 1U;
    if (!BSP_Capture_Init()) app_start_errors |= 2U;
    if (!BSP_PWM_Start()) app_start_errors |= 4U;
    if (app_start_errors) { Error_Handler(); return; }
    now = HAL_GetTick();
    previous_ms = now;
    BSP_Keys_Init(&keys, now);
    BSP_Periodic_Init(&key_timer, now);
    BSP_Periodic_Init(&sample_timer, now);
    BSP_Periodic_Init(&control_timer, now);
    BSP_Periodic_Init(&total_timer, now);
    BSP_Periodic_Init(&screen_timer, now);
    Display_Render(&app_settings, &app_measurements, BSP_PWM_GetDuty(), app_total.litres);
}
void App_Run(void)
{
    uint32_t now = HAL_GetTick(), gap = (uint32_t)(now - previous_ms);
    BSP_KeyEvent events[BSP_KEY_COUNT];
    uint8_t count, i, redraw = 0U;
    if (app_start_errors) return;
    previous_ms = now;
    if (gap > app_max_loop_gap_ms) app_max_loop_gap_ms = gap;
    if (Due(&sample_timer, now, 10U)) { ReadPressure(); ReadFlow(); }
    if (Due(&key_timer, now, 5U)) {
        count = BSP_Keys_Scan(&keys, HAL_GetTick(), events);
        for (i = 0U; i < count; ++i) redraw |= HandleEvent(&events[i]);
    }
    /* 事件处理可能重设控制/积分计时器；重新取now，避免旧now减新起点。 */
    now = HAL_GetTick();
    if (Due(&control_timer, now, 10U)) {
        Control_Step10ms(&app_control, app_measurements.pressure_bar, app_measurements.pressure_valid);
        BSP_PWM_SetDuty(app_control.duty_centi_pct);
    }
    if (Due(&total_timer, now, 100U)) Totalizer_Sample100ms(&app_total, app_measurements.flow_lpm);
    app_led_mask = Alarms_Update(&app_alarms, &app_settings.safety, &app_measurements,
                                 BSP_PWM_GetDuty(), app_control.tracking, now);
    BSP_LED_Write(app_led_mask);
    if (Due(&screen_timer, now, 100U) || redraw)
        Display_Render(&app_settings, &app_measurements, BSP_PWM_GetDuty(), app_total.litres);
    BSP_LCD_Service();            /* 每轮最多一个变化字符；共用总线只在主循环访问。 */
}
''')
print('Wrote phase4 production modules to', FW)