#ifndef DISPLAY_H
#define DISPLAY_H
#include "settings.h"
#include "bsp_lcd.h"
typedef struct {
    char line[BSP_LCD_ROWS][BSP_LCD_COLS + 1U];
} MonitorScreen;
/* Build只构造文字，可在PC核对行列；Render交给同一份LCD字符服务。 */
void Display_Build(MonitorScreen *screen, const Settings *s,
                   const MonitorMeasurements *m, uint16_t actual_duty, uint32_t litres);
void Display_Render(const Settings *s, const MonitorMeasurements *m,
                    uint16_t actual_duty, uint32_t litres);
#endif
