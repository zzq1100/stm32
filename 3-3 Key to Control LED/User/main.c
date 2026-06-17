#include "stm32f10x.h"
#include "Delay.h"
#include "LED.h"
#include "Key.h"

uint8_t keynum = 0;

int main()
{
    LED_Init();
    KEY_Init();
    LED_OFF(0);
    LED_OFF(1);
    while (1)
    {
        keynum = KeyNum();
        if (keynum == 1)
        {
            LED_Turn(0);
        }
        if (keynum == 11)
        {
            LED_Turn(1);
        }
    }
}
