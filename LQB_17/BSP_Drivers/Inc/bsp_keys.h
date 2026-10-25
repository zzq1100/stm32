#ifndef BSP_KEYS_H
#define BSP_KEYS_H
#include "monitor.h"
typedef struct {
    uint8_t raw;         /* 最近一次采样状态：1=按下，0=松开，尚未经过防抖确认 */
    uint8_t stable;      /* 防抖确认后的状态：1=按下，0=松开；状态变化时触发事件 */
    uint8_t valid;       /* 本次按键是否允许发送松开事件：1=允许；按住期间离开原页面后置0 */
    uint32_t changed;   /* 最近一次采样状态变化的时刻，单位ms；用于判断状态是否已保持20ms */
    uint32_t pressed;   /* 本次有效按下的起始时刻，单位ms；确认按下时保存changed，用于计算按住时长 */
    MonitorPage origin; /* 本次确认按下时所在的页面；用于防止跨页面松手误触发功能 */
} KeyState;
typedef struct { KeyState key[4]; } Keys;
void Keys_Init(Keys *keys);
void Keys_Scan(Keys *keys, Monitor *m, uint8_t bits, uint32_t now);
#endif
