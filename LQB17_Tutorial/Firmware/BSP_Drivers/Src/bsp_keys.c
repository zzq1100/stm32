#include "bsp_keys.h"
#include "main.h"

static uint8_t ReadMask(void)
{
    uint8_t mask = 0U;
    /* 板上按键低有效；在入口转为“1表示按下”，后面只处理逻辑状态。 */
    if (HAL_GPIO_ReadPin(KEY_B1_GPIO_Port, KEY_B1_Pin) == GPIO_PIN_RESET) mask |= 1U;
    if (HAL_GPIO_ReadPin(KEY_B2_GPIO_Port, KEY_B2_Pin) == GPIO_PIN_RESET) mask |= 2U;
    if (HAL_GPIO_ReadPin(KEY_B3_GPIO_Port, KEY_B3_Pin) == GPIO_PIN_RESET) mask |= 4U;
    if (HAL_GPIO_ReadPin(KEY_B4_GPIO_Port, KEY_B4_Pin) == GPIO_PIN_RESET) mask |= 8U;
    return mask;
}

void BSP_Keys_Init(BSP_Keys *keys, uint32_t now_ms)
{
    uint8_t i;
    uint8_t mask = ReadMask();
    for (i = 0U; i < BSP_KEY_COUNT; ++i) {
        BSP_KeyState *k = &keys->state[i];
        k->raw = k->stable = (uint8_t)((mask >> i) & 1U);
        k->active = 0U;             /* 开机已有电平不等于观察到一次按下。 */
        k->changed_ms = k->pressed_ms = now_ms;
    }
}

uint8_t BSP_Keys_Scan(BSP_Keys *keys, uint32_t now_ms,
                      BSP_KeyEvent events[BSP_KEY_COUNT])
{
    uint8_t i, count = 0U;
    uint8_t mask = ReadMask();
    for (i = 0U; i < BSP_KEY_COUNT; ++i) {
        BSP_KeyState *k = &keys->state[i];
        uint8_t raw = (uint8_t)((mask >> i) & 1U);

        /* 需求P06-05：任何抖动都重启“连续稳定20ms”的计时。 */
        if (raw != k->raw) {
            k->raw = raw;
            k->changed_ms = now_ms;
        }
        if (raw == k->stable ||
            (uint32_t)(now_ms - k->changed_ms) < BSP_KEY_DEBOUNCE_MS) {
            continue;              /* 未出现新动作，或新电平还没有稳定。 */
        }

        k->stable = raw;
        if (raw != 0U) {
            k->active = 1U;
            k->pressed_ms = k->changed_ms;
            events[count].key = (uint8_t)(i + 1U);
            events[count].type = BSP_KEY_PRESS;
            events[count++].held_ms = 0U;
        } else if (k->active != 0U) {
            k->active = 0U;
            events[count].key = (uint8_t)(i + 1U);
            events[count].type = BSP_KEY_RELEASE;
            /* 两端都取稳定区间起点，避免把松开防抖额外算成长按。 */
            events[count++].held_ms = (uint32_t)(k->changed_ms - k->pressed_ms);
        }
    }
    return count;
}

uint8_t BSP_Keys_PressedMask(const BSP_Keys *keys)
{
    uint8_t i, mask = 0U;
    for (i = 0U; i < BSP_KEY_COUNT; ++i) {
        if (keys->state[i].stable != 0U) mask |= (uint8_t)(1U << i);
    }
    return mask;
}
