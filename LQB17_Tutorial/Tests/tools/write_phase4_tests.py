from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def put(rel,s):
    p=ROOT/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(s.strip()+'\n',encoding='utf-8')
# 连续报警一旦成立便保持到条件解除，避免连续运行一个tick周期后短暂熄灭。
for rel in ('Firmware/BSP_Drivers/Inc/alarms.h','Tests/tools/write_phase4_sources.py'):
    p=ROOT/rel;s=p.read_text(encoding='utf-8');s=s.replace('uint8_t active;               /* 不能用since_ms==0判断未开始，0也是合法时刻。 */','uint8_t active, asserted;     /* active记录起点有效；asserted保持已确认报警。 */');p.write_text(s,encoding='utf-8')
for rel in ('Firmware/BSP_Drivers/Src/alarms.c','Tests/tools/write_phase4_sources.py'):
    p=ROOT/rel;s=p.read_text(encoding='utf-8')
    s=s.replace('if (!condition) { d->active = 0U; return 0U; }','if (!condition) { d->active = d->asserted = 0U; return 0U; }')
    s=s.replace('if (!d->active) { d->active = 1U; d->since_ms = now; }','if (!d->active) { d->active = 1U; d->asserted = 0U; d->since_ms = now; }')
    s=s.replace('return (uint8_t)((uint32_t)(now - d->since_ms) > 2000U);','if ((uint32_t)(now - d->since_ms) > 2000U) d->asserted = 1U;\n    return d->asserted;            /* 持续超过一次tick回绕也不重新熄灭。 */')
    p.write_text(s,encoding='utf-8')
put('Tests/host4_stubs/main.h',r'''
#ifndef PHASE4_TEST_MAIN_H
#define PHASE4_TEST_MAIN_H
#include <stdint.h>
extern uint32_t SystemCoreClock;
uint32_t HAL_GetTick(void);
void Error_Handler(void);
#endif
''')
put('Tests/test_phase4.c',r'''
/* 测真实业务模块与最终App；仅替换硬件BSP，不把预期当作实物测量。 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "settings.h"
#include "control.h"
#include "totalizer.h"
#include "alarms.h"
#include "display.h"
#include "app.h"
#include "bsp_adc.h"
#include "bsp_capture.h"
#include "bsp_pwm.h"
#include "bsp_led.h"
#include "main.h"

extern Settings app_settings;
extern Control app_control;
extern Totalizer app_total;
extern MonitorMeasurements app_measurements;
extern volatile uint8_t app_start_errors, app_zero_result, app_led_mask;
extern volatile uint32_t app_missed_slots, app_max_loop_gap_ms;
uint32_t SystemCoreClock = 80000000U;
static uint32_t tick, adc_reads, fault_calls, adc_delay;
static uint8_t adc_valid=1U, capture_valid=1U, start_adc=1U, start_capture=1U, start_pwm=1U;
static float voltage=1.65f, frequency=6000.0f;
static uint16_t compare;
static uint8_t led_mask, queued;
static BSP_KeyEvent event_queue[4];
static char lcd_lines[10][21];

uint32_t HAL_GetTick(void) { return tick; }
void Error_Handler(void) { ++fault_calls; }
void BSP_LED_Init(void) { led_mask=0U; }
void BSP_LED_Write(uint8_t mask) { led_mask=mask; assert((mask&0xf0U)==0U); }
void BSP_LCD_Init(void) { memset(lcd_lines,0,sizeof(lcd_lines)); }
void BSP_LCD_ClearBuffer(void) { memset(lcd_lines,0,sizeof(lcd_lines)); }
void BSP_LCD_SetLine(uint8_t row,const char *text) { assert(row<10U); (void)snprintf(lcd_lines[row],21U,"%-20.20s",text); }
void BSP_LCD_Service(void) { }
uint8_t BSP_ADC_Init(void) { return start_adc; }
uint8_t BSP_ADC_Read(BSP_ADC_Sample *s) {
    ++adc_reads;tick+=adc_delay;
    s->valid=adc_valid;s->voltage_v=adc_valid?voltage:0.0f;
    s->raw=adc_valid?(uint16_t)(voltage*4095.0f/3.3f):0U;return adc_valid;
}
uint8_t BSP_Capture_Init(void) { return start_capture; }
void BSP_Capture_Read(BSP_CaptureSample *s) {
    memset(s,0,sizeof(*s));s->valid=capture_valid;s->frequency_hz=capture_valid?frequency:0.0f;
}
void BSP_PWM_SetDuty(uint16_t d) { assert(d>=500U&&d<=9500U);compare=(uint16_t)((8000UL*d+5000U)/10000U); }
uint8_t BSP_PWM_Start(void) { BSP_PWM_SetDuty(500U);return start_pwm; }
uint16_t BSP_PWM_GetDuty(void) { return (uint16_t)((compare*10000UL+4000U)/8000U); }
void BSP_Keys_Init(BSP_Keys *keys,uint32_t now) { (void)now;memset(keys,0,sizeof(*keys));queued=0U; }
uint8_t BSP_Keys_Scan(BSP_Keys *keys,uint32_t now,BSP_KeyEvent events[4]) {
    uint8_t count=queued;(void)keys;(void)now;memcpy(events,event_queue,count*sizeof(*events));queued=0U;return count;
}
uint8_t BSP_Keys_PressedMask(const BSP_Keys *keys) { (void)keys;return 0U; }
static void near_value(double a,double b,double tolerance) { assert(fabs(a-b)<=tolerance); }
static uint8_t key(Settings *s,uint8_t n,BSP_KeyEventType type,uint32_t held) {
    BSP_KeyEvent e={n,type,held};return Settings_Handle(s,&e);
}
static void press(Settings *s,uint8_t n) { (void)key(s,n,BSP_KEY_PRESS,0U); }
static void test_settings(void) {
    Settings s;unsigned i,j;uint8_t r;
    Settings_Init(&s);
    assert(s.page==MONITOR_MAIN&&s.mode==MONITOR_MAN&&s.output.tar_d_pct==5U&&s.output.tar_p_deci==50U);
    assert(s.safety.fh_deci==200U&&s.safety.fl_deci==100U&&s.safety.pl_deci==10U&&s.safety.dh_pct==65U);
    assert(key(&s,2U,BSP_KEY_PRESS,0U)==SETTINGS_MODE_CHANGED&&s.mode==MONITOR_AUTO);
    press(&s,1U);assert(s.page==MONITOR_OUTP&&s.selected==0U);
    press(&s,3U);assert(s.output_draft.tar_d_pct==10U&&s.output.tar_d_pct==5U);
    press(&s,2U);press(&s,3U);assert(s.output_draft.tar_p_deci==55U&&s.mode==MONITOR_AUTO);
    r=key(&s,1U,BSP_KEY_PRESS,0U);assert((r&SETTINGS_OUTPUT_CHANGED)&&s.output.tar_d_pct==10U&&s.output.tar_p_deci==55U);
    assert(s.page==MONITOR_PARA&&s.selected==0U);
    press(&s,3U);assert(s.safety_draft.fh_deci==210U&&s.safety.fh_deci==200U);
    r=key(&s,1U,BSP_KEY_PRESS,0U);assert((r&SETTINGS_PARAMS_CHANGED)&&s.safety.fh_deci==210U&&s.page==MONITOR_MAIN);
    press(&s,1U);assert(s.selected==0U);press(&s,1U);assert(s.selected==0U);
    r=key(&s,1U,BSP_KEY_PRESS,0U);assert(!(r&SETTINGS_PARAMS_CHANGED));
    /* 六项范围、步长与限幅；FH和FL允许独立取值。 */
    Settings_Init(&s);press(&s,1U);
    for(i=0;i<40U;++i)press(&s,3U);assert(s.output_draft.tar_d_pct==95U);
    for(i=0;i<40U;++i)press(&s,4U);assert(s.output_draft.tar_d_pct==5U);
    press(&s,2U);for(i=0;i<40U;++i)press(&s,3U);assert(s.output_draft.tar_p_deci==95U);
    for(i=0;i<40U;++i)press(&s,4U);assert(s.output_draft.tar_p_deci==10U);
    press(&s,1U);
    for(j=0;j<4U;++j) {
        for(i=0;i<50U;++i)press(&s,3U);
        if(j==0U)assert(s.safety_draft.fh_deci==400U);
        if(j==1U)assert(s.safety_draft.fl_deci==400U);
        if(j==2U)assert(s.safety_draft.pl_deci==95U);
        if(j==3U)assert(s.safety_draft.dh_pct==95U);
        for(i=0;i<50U;++i)press(&s,4U);
        if(j==0U)assert(s.safety_draft.fh_deci==40U);
        if(j==1U)assert(s.safety_draft.fl_deci==40U);
        if(j==2U)assert(s.safety_draft.pl_deci==10U);
        if(j==3U)assert(s.safety_draft.dh_pct==5U);
        press(&s,2U);
    }
    assert(s.selected==0U);
    for(i=1999U;i<=2001U;++i) {
        Settings_Init(&s);press(&s,3U);
        assert(key(&s,3U,BSP_KEY_RELEASE,i)==(i>=2000U?SETTINGS_ZERO_REQUEST:0U));
        assert(!key(&s,3U,BSP_KEY_RELEASE,i));
        press(&s,4U);assert(key(&s,4U,BSP_KEY_RELEASE,i)==(i>=2000U?SETTINGS_CLEAR_REQUEST:0U));
    }
    Settings_Init(&s);press(&s,3U);press(&s,1U);press(&s,1U);press(&s,1U);
    assert(!key(&s,3U,BSP_KEY_RELEASE,3000U)); /* 绕回MAIN也不能恢复资格。 */
    press(&s,1U);press(&s,4U);press(&s,1U);press(&s,1U);assert(!key(&s,4U,BSP_KEY_RELEASE,3000U));
    assert(!key(&s,0U,BSP_KEY_PRESS,0U));assert(!key(&s,5U,BSP_KEY_PRESS,0U));
    puts("PASS SETTINGS: defaults, drafts/commit, six ranges, selectors, mode, release-only, cross-page cancellation");
}
static void test_control(void) {
    Control c;OutputConfig target={10U,50U};unsigned i;
    Control_Init(&c);assert(c.duty_centi_pct==500U&&!c.tracking);
    assert(Control_Configure(&c,MONITOR_MAN,&target,0.0f,0U));assert(c.tracking);
    for(i=0;i<100U;++i)Control_Step10ms(&c,0.0f,0U);assert(c.duty_centi_pct==600U);
    for(i=0;i<400U;++i)Control_Step10ms(&c,0.0f,0U);assert(c.duty_centi_pct==1000U&&!c.tracking);
    target.tar_p_deci=60U;assert(!Control_Configure(&c,MONITOR_MAN,&target,0.0f,0U));assert(!c.tracking);
    target.tar_d_pct=5U;(void)Control_Configure(&c,MONITOR_MAN,&target,0.0f,0U);
    for(i=0;i<500U;++i)Control_Step10ms(&c,0.0f,0U);assert(c.duty_centi_pct==500U&&!c.tracking);
    target.tar_p_deci=50U;assert(Control_Configure(&c,MONITOR_AUTO,&target,4.5f,1U));assert(!c.tracking);
    Control_Step10ms(&c,4.5f,1U);Control_Step10ms(&c,5.5f,1U);assert(c.duty_centi_pct==500U);
    Control_Step10ms(&c,4.49f,1U);assert(c.duty_centi_pct==501U&&!c.tracking);
    Control_Step10ms(&c,5.51f,1U);assert(c.duty_centi_pct==500U);
    Control_Step10ms(&c,0.0f,0U);Control_Step10ms(&c,NAN,1U);assert(c.duty_centi_pct==500U);
    target.tar_p_deci=60U;(void)Control_Configure(&c,MONITOR_AUTO,&target,0.0f,1U);assert(c.tracking);
    for(i=0;i<10000U;++i)Control_Step10ms(&c,0.0f,1U);assert(c.duty_centi_pct==9500U&&c.tracking);
    Control_Step10ms(&c,6.0f,1U);assert(!c.tracking);
    for(i=0;i<10000U;++i)Control_Step10ms(&c,10.0f,1U);assert(c.duty_centi_pct==500U&&!c.tracking);
    puts("PASS CONTROL: 1pct/s both directions, deadband endpoints, limits, invalid pressure, target tracking lifecycle");
}
static void test_totalizer(void) {
    Totalizer q;unsigned i;float flow=5.123456f;
    Totalizer_Clear(&q);for(i=0;i<14U;++i)Totalizer_Sample100ms(&q,40.0f);assert(q.litres==0U&&q.remainder==560000000U);
    Totalizer_Sample100ms(&q,40.0f);assert(q.litres==1U&&q.remainder==0U);
    Totalizer_Clear(&q);for(i=0;i<149U;++i)Totalizer_Sample100ms(&q,4.0f);assert(!q.litres);
    Totalizer_Sample100ms(&q,4.0f);assert(q.litres==1U&&!q.remainder);
    Totalizer_Clear(&q);for(i=0;i<10000U;++i)Totalizer_Sample100ms(&q,flow);
    near_value(q.litres+q.remainder/600000000.0,(double)flow*10000.0/600.0,0.00001);
    q.litres=9999999U;q.remainder=560000000U;Totalizer_Sample100ms(&q,40.0f);assert(q.litres==10000000U&&!q.remainder);
    q.litres=UINT32_MAX-1U;q.remainder=560000000U;Totalizer_Sample100ms(&q,40.0f);assert(q.litres==UINT32_MAX&&!q.remainder);
    Totalizer_Sample100ms(&q,40.0f);assert(q.litres==UINT32_MAX&&!q.remainder);
    Totalizer_Clear(&q);Totalizer_Sample100ms(&q,-1.0f);Totalizer_Sample100ms(&q,NAN);Totalizer_Sample100ms(&q,INFINITY);assert(!q.litres&&!q.remainder);
    Totalizer_Sample100ms(&q,100.0f);assert(q.remainder==40000000U);Totalizer_Clear(&q);assert(!q.litres&&!q.remainder);
    puts("PASS TOTALIZER: 100ms units, exact litre carry, fractional flow, 9999999+, saturation, full reset");
}
static void test_alarms(void) {
    Alarms a;SafetyConfig cfg={200U,100U,10U,65U};MonitorMeasurements m={0};uint8_t mask;uint32_t start;
    Alarms_Init(&a);m.capture_valid=1U;m.pressure_valid=1U;m.pressure_bar=2.0f;m.frequency_hz=8000.0f;m.flow_lpm=40.0f;
    assert(!Alarms_Update(&a,&cfg,&m,500U,0U,0U));m.frequency_hz=8000.1f;assert(Alarms_Update(&a,&cfg,&m,500U,0U,1U)==1U);
    m.frequency_hz=8000.0f;m.flow_lpm=10.0f;assert(!Alarms_Update(&a,&cfg,&m,6501U,0U,2U));
    m.flow_lpm=9.9f;assert(!Alarms_Update(&a,&cfg,&m,6500U,0U,3U));
    Alarms_Init(&a);assert(!Alarms_Update(&a,&cfg,&m,6501U,0U,0U));
    assert(!Alarms_Update(&a,&cfg,&m,6501U,0U,2000U));assert(Alarms_Update(&a,&cfg,&m,6501U,0U,2001U)==2U);
    assert(Alarms_Update(&a,&cfg,&m,6501U,0U,0U)==2U); /* 报警成立后即使tick整圈也保持。 */
    m.flow_lpm=10.0f;assert(!Alarms_Update(&a,&cfg,&m,6501U,0U,2100U));
    m.flow_lpm=9.9f;assert(!Alarms_Update(&a,&cfg,&m,6501U,0U,2200U));assert(!Alarms_Update(&a,&cfg,&m,6501U,0U,4200U));
    assert(Alarms_Update(&a,&cfg,&m,6501U,0U,4201U)==2U);
    cfg.fh_deci=210U;assert(Alarms_Update(&a,&cfg,&m,6501U,0U,4202U)==2U); /* 无关阈值不打断。 */
    cfg.fl_deci=110U;assert(!Alarms_Update(&a,&cfg,&m,6501U,0U,4203U));
    Alarms_Init(&a);start=UINT32_MAX-1000U;
    assert(!Alarms_Update(&a,&cfg,&m,6501U,0U,start));assert(!Alarms_Update(&a,&cfg,&m,6501U,0U,start+2000U));
    assert(Alarms_Update(&a,&cfg,&m,6501U,0U,start+2001U)==2U);
    Alarms_Init(&a);cfg.fh_deci=200U;m.flow_lpm=20.0f;m.pressure_bar=0.5f;
    assert(!Alarms_Update(&a,&cfg,&m,500U,0U,0U));m.flow_lpm=20.1f;m.pressure_bar=1.0f;
    assert(!Alarms_Update(&a,&cfg,&m,500U,0U,1U));m.pressure_bar=0.99f;
    assert(!Alarms_Update(&a,&cfg,&m,500U,0U,2U));assert(!Alarms_Update(&a,&cfg,&m,500U,0U,2002U));
    mask=Alarms_Update(&a,&cfg,&m,500U,1U,2003U);assert(mask==12U);
    m.pressure_valid=0U;assert(Alarms_Update(&a,&cfg,&m,500U,1U,2004U)==8U);
    m.capture_valid=0U;m.frequency_hz=9000.0f;assert(!Alarms_Update(&a,&cfg,&m,500U,0U,2005U));
    puts("PASS ALARMS: strict thresholds, 2000/2001ms, recovery/restart, tick wrap, persistent alarms, parameter changes");
}
static void test_display(void) {
    Settings s;MonitorMeasurements m={0};MonitorScreen screen;unsigned row;
    Settings_Init(&s);m.pressure_bar=10.0f;m.flow_lpm=40.0f;m.offset_v=0.2f;
    Display_Build(&screen,&s,&m,8000U,9999999U);
    assert(!strcmp(screen.line[1],"       MAIN         "));assert(!strncmp(screen.line[3],"   M=MAN",8U));
    assert(!strncmp(screen.line[4],"   P=10.0BAR",11U));assert(!strncmp(screen.line[5],"   F=40.0L/M",12U));
    assert(!strncmp(screen.line[6],"   Q=9999999L",13U));assert(!strncmp(screen.line[7],"   D=80%",8U));
    assert(!strncmp(screen.line[8],"   V=0.2V",9U));
    for(row=0;row<10U;++row)assert(strlen(screen.line[row])==20U);
    Display_Render(&s,&m,500U,UINT32_MAX);assert(!strncmp(lcd_lines[6],"   Q=4294967295L",15U));
    s.mode=MONITOR_AUTO;Display_Render(&s,&m,500U,0U);s.mode=MONITOR_MAN;m.pressure_bar=0.0f;Display_Render(&s,&m,500U,0U);
    assert(lcd_lines[3][8]==' ');assert(!strncmp(lcd_lines[4],"   P=0.0BAR ",11U));
    press(&s,1U);press(&s,3U);Display_Build(&screen,&s,&m,500U,0U);
    assert(!strncmp(screen.line[1],"       OUTP",11U));assert(!strncmp(screen.line[3],"   TarD=10%",11U));
    assert(!strncmp(screen.line[4],"   TarP=5.0BAR",13U));assert(!strcmp(screen.line[8],"                    "));
    press(&s,1U);Display_Build(&screen,&s,&m,500U,0U);
    assert(!strncmp(screen.line[3],"   FH=20.0L/M",13U));assert(!strncmp(screen.line[6],"   DH=65%",9U));
    puts("PASS DISPLAY: exact title/data coordinates, units/case, drafts, 20-column padding, long totals, shortened fields");
}
static void reset_app(void) {
    tick=adc_reads=fault_calls=adc_delay=0U;adc_valid=capture_valid=start_adc=start_capture=start_pwm=1U;
    voltage=1.65f;frequency=6000.0f;SystemCoreClock=80000000U;App_Init();assert(!app_start_errors&&!fault_calls);
}
static void run_ms(uint32_t duration) { uint32_t i;for(i=0U;i<duration;++i){++tick;App_Run();} }
static void queue_event(uint8_t n,BSP_KeyEventType type,uint32_t held) {
    assert(queued<4U);event_queue[queued].key=n;event_queue[queued].type=type;event_queue[queued].held_ms=held;++queued;
}
static void send_key(uint8_t n,BSP_KeyEventType type,uint32_t held) { queue_event(n,type,held);run_ms(5U);assert(!queued); }
static void test_app(void) {
    uint32_t reads;uint16_t before;
    reset_app();assert(app_settings.page==MONITOR_MAIN&&app_settings.mode==MONITOR_MAN&&compare==400U&&!app_total.litres);
    assert(!strcmp(lcd_lines[1],"       MAIN         "));run_ms(10U);near_value(app_measurements.pressure_bar,5.0,0.0001);
    send_key(1U,BSP_KEY_PRESS,0U);send_key(3U,BSP_KEY_PRESS,0U);assert(app_control.duty_centi_pct==500U&&app_settings.output.tar_d_pct==5U);
    send_key(1U,BSP_KEY_PRESS,0U);assert(app_settings.output.tar_d_pct==10U&&app_control.tracking&&app_control.duty_centi_pct==500U);
    run_ms(1000U);assert(app_control.duty_centi_pct==600U&&(led_mask&8U));
    run_ms(4000U);assert(app_control.duty_centi_pct==1000U&&!(led_mask&8U));assert(!app_missed_slots);
    reset_app();send_key(3U,BSP_KEY_PRESS,0U);run_ms(2005U);assert(app_measurements.offset_v==0.0f);
    reads=adc_reads;voltage=1.2f;send_key(3U,BSP_KEY_RELEASE,2000U);
    assert(adc_reads>reads&&app_zero_result==1U);near_value(app_measurements.offset_v,1.2,0.00001);near_value(app_measurements.pressure_bar,0.0,0.00001);
    reset_app();send_key(3U,BSP_KEY_PRESS,0U);send_key(1U,BSP_KEY_PRESS,0U);send_key(1U,BSP_KEY_PRESS,0U);send_key(1U,BSP_KEY_PRESS,0U);
    send_key(3U,BSP_KEY_RELEASE,3000U);assert(app_zero_result==0U&&app_measurements.offset_v==0.0f);
    reset_app();app_total.litres=100U;app_total.remainder=123U;
    send_key(4U,BSP_KEY_PRESS,0U);adc_delay=2U;send_key(4U,BSP_KEY_RELEASE,2000U);adc_delay=0U;
    assert(!app_total.litres&&!app_total.remainder);assert(app_missed_slots<10U); /* 无旧now减新计时器下溢。 */
    reset_app();send_key(2U,BSP_KEY_PRESS,0U);voltage=0.0f;run_ms(100U);assert(app_control.duty_centi_pct>500U);
    before=app_control.duty_centi_pct;adc_valid=0U;run_ms(100U);assert(app_control.duty_centi_pct==before);
    reset_app();frequency=8001.0f;run_ms(10U);assert(led_mask&1U);frequency=8000.0f;run_ms(10U);assert(!(led_mask&1U));
    reset_app();frequency=8000.0f;tick=1000U;App_Run();assert(app_total.litres==0U&&app_total.remainder==40000000U);
    assert(app_missed_slots>0U&&app_max_loop_gap_ms==1000U);
    reset_app();tick=UINT32_MAX-50U;App_Init();run_ms(150U);assert(!app_missed_slots&&app_total.remainder==30000000U);
    reset_app();start_adc=0U;App_Init();assert(app_start_errors==1U&&fault_calls==1U);App_Run();
    puts("PASS APP: real final entry, startup, commit/ramp/LD4, fresh release calibration, cross-page cancellation, reset, invalid ADC, overruns, tick wrap");
}
int main(void) {
    test_settings();test_control();test_totalizer();test_alarms();test_display();test_app();
    puts("ALL PHASE4 HOST TESTS PASSED");return 0;
}
''')
put('Tests/run_phase4_tests.ps1',r'''
param([string]$GccPath = 'D:\Downloads\mingw64\bin\gcc.exe')
$ErrorActionPreference = 'Stop'
$firmwareRoot = Join-Path $PSScriptRoot '..\Firmware'
$buildRoot = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
$exePath = Join-Path $buildRoot 'test_phase4.exe'
$arguments = @('-std=c99', '-Wall', '-Wextra', '-Werror', '-pedantic',
    '-I', (Join-Path $PSScriptRoot 'host4_stubs'),
    '-I', (Join-Path $firmwareRoot 'BSP_Drivers\Inc'),
    (Join-Path $PSScriptRoot 'test_phase4.c'))
foreach ($module in @('settings','control','totalizer','alarms','display','app','measurement','bsp_time')) {
    $arguments += Join-Path $firmwareRoot "BSP_Drivers\Src\$module.c"
}
$arguments += @('-lm', '-o', $exePath)
& $GccPath @arguments
if ($LASTEXITCODE -ne 0) { throw "Host compilation failed: $LASTEXITCODE" }
& $exePath | Tee-Object -FilePath (Join-Path $buildRoot 'host_phase4_results.txt')
if ($LASTEXITCODE -ne 0) { throw "Host tests failed: $LASTEXITCODE" }
''')
print('Wrote host tests and persistent alarm latch.')