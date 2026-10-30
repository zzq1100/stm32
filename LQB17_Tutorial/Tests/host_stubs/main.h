#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
typedef struct { uint32_t ODR; } GPIO_TypeDef;
extern GPIO_TypeDef test_gpio_c, test_gpio_d;
#define GPIOC (&test_gpio_c)
#define LED_LE_GPIO_Port (&test_gpio_d)
#define LED_LE_Pin 4U
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET = 1 } GPIO_PinState;
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
#define __NOP() ((void)0)
#define __DSB() ((void)0)
#endif
