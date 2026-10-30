#include "bsp_lcd.h"
#include "lcd.h"
#include <string.h>

static char wanted[BSP_LCD_ROWS][BSP_LCD_COLS];
static char shown[BSP_LCD_ROWS][BSP_LCD_COLS];
static uint16_t cursor;

void BSP_LCD_ClearBuffer(void)
{
    memset(wanted, ' ', sizeof(wanted));
}

void BSP_LCD_Init(void)
{
    LCD_Init();                    /* 官方驱动完成 LCD GPIO 和控制器初始化。 */
    LCD_SetBackColor(Black);
    LCD_SetTextColor(White);
    LCD_Clear(Black);              /* 只在初始化清整屏，运行时更新变化字符。 */
    BSP_LCD_ClearBuffer();
    memset(shown, ' ', sizeof(shown));
    cursor = 0U;
}

void BSP_LCD_SetLine(uint8_t row, const char *text)
{
    uint8_t col;
    uint8_t value;

    if (row >= BSP_LCD_ROWS || text == 0) {
        return;
    }

    for (col = 0U; col < BSP_LCD_COLS; ++col) {
        if (*text == '\0') {
            wanted[row][col] = ' '; /* AUTO 改为 MAN 时，旧的 O 也会被擦除。 */
        } else {
            value = (uint8_t)*text++;
            /* 官方字库只支持 0x20~0x7E，防止索引越出字库。 */
            wanted[row][col] = (value >= 0x20U && value <= 0x7EU)
                              ? (char)value : '?';
        }
    }
}

void BSP_LCD_Service(void)
{
    uint16_t checked;
    uint8_t row;
    uint8_t col;

    for (checked = 0U; checked < BSP_LCD_ROWS * BSP_LCD_COLS; ++checked) {
        row = (uint8_t)(cursor / BSP_LCD_COLS);
        col = (uint8_t)(cursor % BSP_LCD_COLS);
        cursor = (uint16_t)((cursor + 1U) % (BSP_LCD_ROWS * BSP_LCD_COLS));

        if (wanted[row][col] != shown[row][col]) {
            /* 官方接口以像素定位，横坐标从 319 向左递减。 */
            LCD_DisplayChar((uint8_t)(row * 24U),
                            (uint16_t)(319U - col * 16U),
                            (uint8_t)wanted[row][col]);
            shown[row][col] = wanted[row][col];
            return;                /* 本轮到此为止，下轮主循环继续画。 */
        }
    }
}
