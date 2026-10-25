#include "monitor.h"
#include "bsp_keys.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void Press(Monitor *m, unsigned key)
{ Monitor_Key(m, (uint8_t)key, 1, 0, m->page); }
static void Near(float a, float b) { assert(fabsf(a-b) < 0.0001f); }

int main(void)
{
    Monitor m;
    Keys keys;
    unsigned i;
    Monitor_Init(&m);
    assert(m.page == PAGE_MAIN && !m.automatic && m.duty == 500);
    assert(m.active.target_d == 5 && m.active.target_p == 50);
    assert(m.active.fh == 200 && m.active.fl == 100 && m.active.pl == 10 && m.active.dh == 65);
    assert(m.offset == 0 && m.litres == 0 && m.leds == 0);

    Monitor_Measure(&m, 1.65f, 800.0f); Near(m.pressure,5); Near(m.flow,4);
    Monitor_Measure(&m, 3.3f, 8000); Near(m.pressure,10); Near(m.flow,40);
    Monitor_Alarms(&m,0); assert(!(m.leds&1));
    Monitor_Measure(&m, 1.0f, 8000.1f); Monitor_Alarms(&m,1); assert(m.leds&1); Near(m.flow,40);
    Monitor_Measure(&m, 1.0f, 799.9f); Monitor_Alarms(&m,2); assert(!(m.leds&1)); Near(m.flow,0);

    /* 必须在主页面长按满 2 秒并松手才校准。 */
    Monitor_Key(&m,3,1,3000,PAGE_MAIN); Near(m.offset,0);
    Monitor_Key(&m,3,0,1999,PAGE_MAIN); Near(m.offset,0);
    Monitor_Key(&m,3,0,2000,PAGE_MAIN); Near(m.offset,1.0f); Near(m.pressure,0);
    Monitor_Measure(&m, 2.15f, 4000); Near(m.pressure,5);
    Monitor_Measure(&m, 0.5f, 4000); Near(m.pressure,0);
    Monitor_Measure(&m, 3.3f, 4000); Monitor_Key(&m,3,0,2000,PAGE_MAIN); Near(m.offset,1);

    Monitor_Init(&m);
    Press(&m,1); assert(m.page == PAGE_OUTP && m.selection == 0);
    Press(&m,3); assert(m.draft.target_d == 10 && m.active.target_d == 5);
    for(i=0;i<100;++i) Monitor_Control10ms(&m);
    assert(m.duty == 500);
    Press(&m,1); assert(m.page == PAGE_PARA && m.active.target_d == 10 && m.adjusting);
    for(i=0;i<100;++i) Monitor_Control10ms(&m);
    assert(m.duty == 600);
    for(i=0;i<400;++i) Monitor_Control10ms(&m);
    assert(m.duty == 1000 && !m.adjusting);
    Press(&m,3); assert(m.draft.fh == 210 && m.active.fh == 200);
    Press(&m,1); assert(m.active.fh == 210 && m.page == PAGE_MAIN);
    Press(&m,1); Press(&m,2); assert(m.selection == 1);
    for(i=0;i<30;++i) Press(&m,3);
    assert(m.draft.target_p == 95);
    for(i=0;i<30;++i) Press(&m,4);
    assert(m.draft.target_p == 10);
    Press(&m,1); Press(&m,2); Press(&m,2); Press(&m,2);
    assert(m.selection == 3);
    for(i=0;i<20;++i) Press(&m,3);
    assert(m.draft.dh == 95);
    for(i=0;i<20;++i) Press(&m,4);
    assert(m.draft.dh == 5);

    Monitor_Init(&m); m.automatic = 1; m.duty = 5000; m.adjusting = 1;
    m.pressure = 4.49f;
    for(i=0;i<100;++i) Monitor_Control10ms(&m);
    assert(m.duty == 5100 && m.adjusting);
    m.pressure=4.5f; Monitor_Control10ms(&m); assert(m.duty==5100 && !m.adjusting);
    m.pressure=5.5f; Monitor_Control10ms(&m); assert(m.duty==5100);
    m.pressure=5.51f; Monitor_Control10ms(&m); assert(m.duty==5099);
    m.duty=500; Monitor_Control10ms(&m); assert(m.duty==500);
    m.duty=9500; m.pressure=0; Monitor_Control10ms(&m); assert(m.duty==9500);

    Monitor_Init(&m); m.flow=40;
    for(i=0;i<14;++i) Monitor_Integrate100ms(&m);
    assert(m.litres==0);
    Monitor_Integrate100ms(&m); assert(m.litres==1);
    m.litres=9999999;
    for(i=0;i<15;++i) Monitor_Integrate100ms(&m);
    assert(m.litres==10000000);
    Monitor_Key(&m,4,0,1999,PAGE_MAIN); assert(m.litres==10000000);
    Monitor_Key(&m,4,0,2000,PAGE_MAIN); assert(m.litres==0 && m.fraction==0);

    Monitor_Init(&m); m.flow=0; m.duty=6500;
    Monitor_Alarms(&m,100); Monitor_Alarms(&m,3000); assert(!(m.leds&2));
    m.duty=6501; Monitor_Alarms(&m,4000); Monitor_Alarms(&m,6000); assert(!(m.leds&2));
    Monitor_Alarms(&m,6001); assert(m.leds&2);
    m.flow=10; Monitor_Alarms(&m,6002); assert(!(m.leds&2));
    m.flow=21; m.pressure=0.9f;
    Monitor_Alarms(&m,7000); Monitor_Alarms(&m,9001); assert(m.leds&4);
    m.pressure=1; Monitor_Alarms(&m,9002); assert(!(m.leds&4));
    m.flow=0; m.duty=7000;
    Monitor_Alarms(&m,UINT32_MAX-1000); Monitor_Alarms(&m,1000); assert(m.leds&2);
    assert(!(m.leds&0xf0));

    /* 模拟抖动、持续按住、跨页面长按，直接运行实际按键模块。 */
    Monitor_Init(&m); Keys_Init(&keys);
    Keys_Scan(&keys,&m,1,0); Keys_Scan(&keys,&m,0,5);
    Keys_Scan(&keys,&m,1,10); Keys_Scan(&keys,&m,1,29);
    assert(m.page==PAGE_MAIN);
    Keys_Scan(&keys,&m,1,30); assert(m.page==PAGE_OUTP);
    Keys_Scan(&keys,&m,1,3000); assert(m.page==PAGE_OUTP);
    Keys_Scan(&keys,&m,0,3005); Keys_Scan(&keys,&m,0,3025);
    Monitor_Init(&m); Keys_Init(&keys); m.voltage=1.2f;
    Keys_Scan(&keys,&m,4,100); Keys_Scan(&keys,&m,4,120);
    Keys_Scan(&keys,&m,4,2200); Near(m.offset,0);
    Keys_Scan(&keys,&m,0,2205); Keys_Scan(&keys,&m,0,2225); Near(m.offset,1.2f);
    Monitor_Init(&m); Keys_Init(&keys); m.voltage=1.2f;
    Keys_Scan(&keys,&m,4,100); Keys_Scan(&keys,&m,4,120);
    Press(&m,1); Keys_Scan(&keys,&m,4,125); Press(&m,1); Press(&m,1);
    Keys_Scan(&keys,&m,0,2200); Keys_Scan(&keys,&m,0,2220); Near(m.offset,0);
    puts("PASS: defaults, pressure/calibration, frequency limits, staged settings, PWM rate, automatic deadband/limits, volume precision, alarm timing/wrap, debounce/long press");
    return 0;
}
