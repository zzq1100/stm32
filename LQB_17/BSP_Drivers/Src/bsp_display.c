#include "bsp_display.h"
#include "lcd.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static char wanted[10][20], shown[10][20];
static unsigned cursor;
static void Row(unsigned row, const char *format, ...)
{
    char buffer[48];
    size_t length;
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    length = strlen(buffer);
    if (length > 20) length = 20;
    memcpy(wanted[row], buffer, length);
}

void Display_Init(void)
{
    LCD_Init();
    LCD_SetBackColor(Black);
    LCD_SetTextColor(White);
    LCD_Clear(Black);
    memset(wanted, ' ', sizeof(wanted));
    memset(shown, ' ', sizeof(shown));
    cursor = 0;
}

void Display_Compose(const Monitor *m)
{
    unsigned p = (unsigned)(m->pressure * 10.0f + 0.5f);
    unsigned f = (unsigned)(m->flow * 10.0f + 0.5f);
    unsigned v = (unsigned)(m->offset * 10.0f + 0.5f);
    const MonitorConfig *c = &m->draft;
    /* 每行固定 20 字符，补空格清除旧数字；行列对应试题示意图。
       不通过反色/闪烁提示选择，遵守前景白、背景黑且按键不改颜色。 */
    memset(wanted, ' ', sizeof(wanted));
    if (m->page == PAGE_MAIN) {
        Row(1, "       MAIN");
        Row(3, "   M=%s", m->automatic ? "AUTO" : "MAN");
        Row(4, "   P=%u.%uBAR", p/10, p%10);
        Row(5, "   F=%u.%uL/M", f/10, f%10);
        Row(6, "   Q=%luL", (unsigned long)m->litres);
        Row(7, "   D=%u%%", (unsigned)((m->duty+50U)/100U));
        Row(8, "   V=%u.%uV", v/10, v%10);
    } else if (m->page == PAGE_OUTP) {
        Row(1, "       OUTP");
        Row(3, "   TarD=%u%%", (unsigned)c->target_d);
        Row(4, "   TarP=%u.%uBAR", c->target_p/10, c->target_p%10);
    } else {
        Row(1, "       PARA");
        Row(3, "   FH=%u.%uL/M", c->fh/10, c->fh%10);
        Row(4, "   FL=%u.%uL/M", c->fl/10, c->fl%10);
        Row(5, "   PL=%u.%uBAR", c->pl/10, c->pl%10);
        Row(6, "   DH=%u%%", (unsigned)c->dh);
    }
}

void Display_Service(void)
{
    unsigned n, row, col;
    /* 每次主循环最多画一个变化的字符，避免整屏刷写阻塞按键和采样。
       不在运行期间 LCD_Clear，因此不会周期性闪屏。 */
    for (n = 0; n < 200; ++n) {
        row = cursor / 20; col = cursor % 20;
        cursor = (cursor + 1) % 200;
        if (shown[row][col] != wanted[row][col]) {
            LCD_DisplayChar((uint8_t)(row*24), (uint16_t)(319-col*16), (uint8_t)wanted[row][col]);
            shown[row][col] = wanted[row][col];
            return;
        }
    }
}
