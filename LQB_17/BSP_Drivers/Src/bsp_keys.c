#include "bsp_keys.h"
#include <string.h>
void Keys_Init(Keys *keys) { memset(keys, 0, sizeof(*keys)); }
/*
 * 扫描四个按键：识别防抖后的按下/松开，并交给 Monitor_Key 处理具体功能。
 * keys：保存四个按键各自的历史状态和计时信息，不能每次扫描都清零。
 * m   ：系统状态，包含当前页面；Monitor_Key 会根据它决定执行什么操作。
 * bits：bit0~bit3 对应 B1~B4；1=按下，0=松开。
 *       BSP_ReadKeys 已把硬件的低电平按下转换成这里的 1，不必再次取反。
 * now ：本次扫描的系统时间，单位 ms，来自 HAL_GetTick()。
 *
 * 本函数由 app.c 按约 5 ms 间隔调用；不使用延时等待，因此按住按键时，
 * 主循环仍能继续采集数据、控制 PWM 和刷新显示。
 */
void Keys_Scan(Keys *keys, Monitor *m, uint8_t bits, uint32_t now)
{
    unsigned i;

    /* 四个按键逐个处理，每个按键使用自己的 KeyState。 */
    for (i = 0; i < 4; ++i) {
        /* k 指向第 i 个按键的状态；修改 k->成员，就是修改 keys->key[i]。 */
        KeyState *k = &keys->key[i];

        /* 将第 i 位右移到最低位，再用 &1 保留这一位。
         * 例如 bits=0x04（二进制0100）、i=2 时，raw=1，表示 B3 按下。
         * 局部变量 raw 是本次采样值，k->raw 是之前保存的采样值。
         */
        uint8_t raw = (bits >> i) & 1U;

        /* 第一步：发现采样状态变化，更新记录，并从此刻重新开始防抖计时。
         * 按键抖动会使 0/1 反复变化，每次变化都会重置 changed。
         */
        if (raw != k->raw) {
            k->raw = raw;
            k->changed = now;
        }

        /* 第二步：只有同时满足以下两点，才确认一次有效的状态变化：
         * 1. raw != stable：采样值与已确认状态不同，存在待确认的变化。
         * 2. 距离最后一次采样变化至少20ms：这段时间未再采到反向变化。
         *
         * 确认后 stable=raw，持续按住时二者相等，不会重复产生按下事件。
         * now、changed 都是无符号时间，使用差值可处理系统毫秒计数回绕。
         */
        if (raw != k->stable && (uint32_t)(now - k->changed) >= 20U) {
            k->stable = raw;

            if (raw) {
                /* 确认按下：记录稳定按下开始的时刻，而非防抖结束时刻。
                 * 使用 changed，可以避免计算按住时长时扣掉按下侧的防抖时间。
                 */
                k->pressed = k->changed;
                k->origin = m->page; /* 记录确认按下时所在的页面 */
                k->valid = 1;        /* 允许本次按键随后发送松开事件 */

                /* i=0~3 对应按键编号1~4；第三个参数1表示“按下”。
                 * 此时还未松开，按住时长传0。
                 * B1切页、B2切换、设置页B3/B4加减，在 Monitor_Key 中执行。
                 */
                Monitor_Key(m, (uint8_t)(i+1), 1, 0, k->origin);
            } else {
                /* 确认松开：仅在本次按键仍有效时发送松开事件。
                 * 第三个参数0表示“松开”；此时 changed 是稳定松开开始时刻，
                 * changed-pressed 就是这次按住的时长，单位ms。
                 *
                 * 本函数只提供时长；Monitor_Key 再判断是否在 MAIN 页面、
                 * 是否按住至少2000ms，并执行B3校准或B4清零。
                 * 因此按住满2秒不会立即执行，必须松手且通过松开防抖。
                 */
                if (k->valid) {
                    Monitor_Key(m, (uint8_t)(i+1), 0,
                                k->changed - k->pressed, k->origin);
                }
                k->valid = 0; /* 本次按键结束，等待下一次有效按下 */
            }
        }
    }

    /* 防止跨页面长按误操作：检查所有仍处于按下状态的按键。
     * 若当前页面与按下时页面不同，就取消这次按键的松开事件。
     * 例如在 MAIN 按住B4，再用B1离开页面，之后即使回到 MAIN，
     * 松开B4也不会清零；需要重新按下才会把 valid 置1。
     * 放在上面的事件处理之后检查，才能看到B1等操作引起的页面变化。
     */
    for (i = 0; i < 4; ++i) {
        if (keys->key[i].stable && keys->key[i].origin != m->page) {
            keys->key[i].valid = 0;
        }
    }
}
