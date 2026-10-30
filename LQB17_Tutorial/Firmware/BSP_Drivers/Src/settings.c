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
