/* 使用生产模块源码，只替换 GPIO/LCD 硬件入口。
 * 能验证逻辑和调用约定，不能证明引脚电平或锁存时序。 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bsp_led.h"
#include "bsp_lcd.h"
#include "bsp_time.h"
#include "main.h"
#include "lcd.h"

GPIO_TypeDef test_gpio_c, test_gpio_d;
static uint8_t latch_open;
static uint32_t latched_bus, latch_count;
static char pixels[10][20];
static uint32_t draw_count, clear_count, lcd_init_count;
static uint16_t text_color, back_color;

void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    assert(port == LED_LE_GPIO_Port && pin == LED_LE_Pin);
    if (state == GPIO_PIN_SET) {
        latch_open = 1U;
    } else if (latch_open) {
        latched_bus = GPIOC->ODR;
        ++latch_count;
        latch_open = 0U;
    }
}
void LCD_Init(void) { ++lcd_init_count; }
void LCD_SetBackColor(uint16_t color) { back_color = color; }
void LCD_SetTextColor(uint16_t color) { text_color = color; }
void LCD_Clear(uint16_t color)
{
    assert(color == Black);
    memset(pixels, ' ', sizeof(pixels));
    ++clear_count;
}
void LCD_DisplayChar(uint8_t line, uint16_t column, uint8_t ascii)
{
    assert(line % 24U == 0U && line <= 216U);
    assert(column <= 319U && (319U-column) % 16U == 0U);
    assert(ascii >= 0x20U && ascii <= 0x7EU);
    pixels[line/24U][(319U-column)/16U] = (char)ascii;
    ++draw_count;
}

static void drain_screen(void)
{
    unsigned i;
    for (i = 0U; i < 200U; ++i) {
        uint32_t before = draw_count;
        BSP_LCD_Service();
        assert(draw_count-before <= 1U); /* 每轮绘制预算必须遵守。 */
    }
}

static void test_led(void)
{
    uint32_t before;
    GPIOC->ODR = 0xABCDU;
    BSP_LED_Init();
    assert(latched_bus == 0xFFCDU && GPIOC->ODR == 0xABCDU);
    assert(!latch_open);
    BSP_LED_Write(1U);
    assert(latched_bus == 0xFECDU && GPIOC->ODR == 0xABCDU);
    GPIOC->ODR = 0x1234U;           /* 模拟 LCD 已经写了别的数据。 */
    BSP_LED_Write(0x0AU);
    assert(latched_bus == 0xF534U && GPIOC->ODR == 0x1234U);
    before = latch_count;
    BSP_LED_Write(0x0AU);
    assert(latch_count == before && GPIOC->ODR == 0x1234U);
    BSP_LED_Init();
    assert(latched_bus == 0xFF34U && !latch_open);
    puts("PASS LED: polarity, bit mapping, bus restore, unchanged mask, reinit");
}

static void test_lcd(void)
{
    uint32_t before;
    unsigned i;
    char local[] = "LOCAL";
    BSP_LCD_Init();
    assert(lcd_init_count == 1U && clear_count == 1U);
    assert(text_color == White && back_color == Black);
    BSP_LCD_SetLine(0U, "AUTO");
    drain_screen();
    assert(memcmp(pixels[0], "AUTO                ", 20U) == 0);
    BSP_LCD_SetLine(0U, "MAN");
    drain_screen();
    assert(memcmp(pixels[0], "MAN                 ", 20U) == 0);
    before = draw_count;
    BSP_LCD_SetLine(0U, "MAN");
    drain_screen();
    assert(draw_count == before);
    BSP_LCD_SetLine(1U, local);
    local[0] = 'X';                /* 验证已复制文本，不保留局部数组指针。 */
    BSP_LCD_SetLine(2U, "12345678901234567890EXTRA");
    BSP_LCD_SetLine(3U, "A\nB\xFF");
    BSP_LCD_SetLine(255U, "BAD");
    BSP_LCD_SetLine(0U, 0);
    drain_screen();
    assert(memcmp(pixels[1], "LOCAL", 5U) == 0);
    assert(memcmp(pixels[2], "12345678901234567890", 20U) == 0);
    assert(memcmp(pixels[3], "A?B?", 4U) == 0);
    BSP_LCD_ClearBuffer();
    drain_screen();
    for (i = 0U; i < sizeof(pixels); ++i) {
        assert(((char *)pixels)[i] == ' ');
    }
    assert(clear_count == 1U);     /* 切换内容不能偷偷执行整屏清空。 */
    BSP_LCD_SetLine(9U, "TAIL");
    for (i = 0U; i < 200U; ++i) {
        BSP_LCD_SetLine(0U, (i & 1U) ? "A" : "B");
        BSP_LCD_Service();
    }
    assert(memcmp(pixels[9], "TAIL", 4U) == 0); /* 首行频繁变化不饿死末行。 */
    puts("PASS LCD: coordinates, padding, copy, bounds, ASCII, budget, fairness");
}

static void test_time(void)
{
    BSP_Periodic a,b,c;
    uint32_t now, ca=0U, cb=0U, cc=0U;
    BSP_Periodic_Init(&a, 100U);
    assert(!BSP_Periodic_Due(&a, 104U, 5U));
    assert(BSP_Periodic_Due(&a, 106U, 5U));
    assert(!BSP_Periodic_Due(&a, 106U, 5U));
    assert(BSP_Periodic_Due(&a, 110U, 5U));
    assert(a.last_ms == 110U);
    assert(!BSP_Periodic_Due(&a, 1000U, 0U) && a.last_ms == 110U);
    BSP_Periodic_Init(&a, 0xFFFFFFF0U);
    assert(BSP_Periodic_Due(&a, 4U, 10U));
    assert(BSP_Periodic_Due(&a, 4U, 10U));
    assert(!BSP_Periodic_Due(&a, 4U, 10U));
    BSP_Periodic_Init(&a, 0U);
    BSP_Periodic_Init(&b, 0U);
    BSP_Periodic_Init(&c, 0U);
    for (now=0U; now<=10000U; ++now) {
        ca += BSP_Periodic_Due(&a, now, 5U);
        cb += BSP_Periodic_Due(&b, now, 10U);
        cc += BSP_Periodic_Due(&c, now, 100U);
    }
    assert(ca == 2000U && cb == 1000U && cc == 100U);
    puts("PASS TIME: boundary, no duplicate, phase, invalid period, wrap, independent tasks");
}

int main(void)
{
    test_led();
    test_lcd();
    test_time();
    puts("ALL PHASE2 HOST TESTS PASSED");
    return 0;
}
