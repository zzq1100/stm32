#ifndef BSP_KEYS_H
#define BSP_KEYS_H

#include <stdint.h>

#define BSP_KEY_COUNT 4U
#define BSP_KEY_DEBOUNCE_MS 20U

typedef enum {
    BSP_KEY_PRESS = 1,             /* 防抖后确认的一次按下。 */
    BSP_KEY_RELEASE                /* 防抖后确认的一次松开。 */
} BSP_KeyEventType;

typedef struct {
    uint8_t key;                   /* B1~B4，编号1~4。 */
    BSP_KeyEventType type;
    uint32_t held_ms;              /* 松开事件的持续时间；按下事件为0。 */
} BSP_KeyEvent;

typedef struct {
    uint8_t raw;                   /* 最近采样的逻辑状态：1按下，0松开。 */
    uint8_t stable;                /* 连续稳定20ms后确认的状态。 */
    uint8_t active;                /* 是否在初始化以后确认过本次按下。 */
    uint32_t changed_ms;           /* raw最后一次变化的时刻，防抖从此计时。 */
    uint32_t pressed_ms;           /* 本次有效按下的稳定区间起点。 */
} BSP_KeyState;

typedef struct {
    BSP_KeyState state[BSP_KEY_COUNT];
} BSP_Keys;

/* GPIO初始化后调用；开机已按住的键先等松开，不产生伪造长按。
 * keys指针必须有效，状态对象需要跨越多次Scan保存。 */
void BSP_Keys_Init(BSP_Keys *keys, uint32_t now_ms);

/* 建议每5ms调用，最多返回4个事件，按B1~B4排序。
 * events必须指向至少4个元素的数组；返回值是本次有效事件数。
 * 只报告物理动作，不知道页面，也不直接校准或清零。 */
uint8_t BSP_Keys_Scan(BSP_Keys *keys, uint32_t now_ms,
                      BSP_KeyEvent events[BSP_KEY_COUNT]);
uint8_t BSP_Keys_PressedMask(const BSP_Keys *keys);

#endif
