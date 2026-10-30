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
