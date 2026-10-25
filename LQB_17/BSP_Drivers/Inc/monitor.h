#ifndef MONITOR_H
#define MONITOR_H
#include <stdint.h>
typedef enum { PAGE_MAIN, PAGE_OUTP, PAGE_PARA } MonitorPage;
typedef struct {
    uint16_t target_d; /* % */
    uint16_t target_p; /* 0.1 BAR */
    uint16_t fh, fl;   /* 0.1 L/MIN */
    uint16_t pl;       /* 0.1 BAR */
    uint16_t dh;       /* % */
} MonitorConfig;
typedef struct {
    MonitorConfig active, draft;
    MonitorPage page;
    uint8_t selection, automatic;
    uint16_t duty; /* 0.01%，用于平滑调节，显示取整 */
    float voltage, offset, pressure, frequency, flow;
    uint32_t litres;
    double fraction;
    uint8_t leds, adjusting;
    uint8_t block_timing, leak_timing;
    uint32_t block_start, leak_start;
} Monitor;
void Monitor_Init(Monitor *m);
void Monitor_Measure(Monitor *m, float voltage, float frequency);
void Monitor_Integrate100ms(Monitor *m);
void Monitor_Control10ms(Monitor *m);
void Monitor_Alarms(Monitor *m, uint32_t now);
/* key=1..4；down=1 为防抖后的按下；down=0 为松开。
   origin_page 是按下时的页面，防止跨页面长按误触发。 */
void Monitor_Key(Monitor *m, uint8_t key, uint8_t down,
                 uint32_t held_ms, MonitorPage origin_page);
#endif
