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
