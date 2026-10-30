#ifndef TEST_HAL_H
#define TEST_HAL_H
#include <stdint.h>
typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef enum { GPIO_PIN_RESET, GPIO_PIN_SET } GPIO_PinState;
typedef struct { uint32_t IDR; } GPIO_TypeDef;
extern GPIO_TypeDef fake_gpio_a, fake_gpio_b;
#define KEY_B1_GPIO_Port (&fake_gpio_b)
#define KEY_B2_GPIO_Port (&fake_gpio_b)
#define KEY_B3_GPIO_Port (&fake_gpio_b)
#define KEY_B4_GPIO_Port (&fake_gpio_a)
#define KEY_B1_Pin 1U
#define KEY_B2_Pin 2U
#define KEY_B3_Pin 4U
#define KEY_B4_Pin 1U
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);
typedef struct { uint32_t unused; } ADC_HandleTypeDef;
extern ADC_HandleTypeDef hadc2;
#define ADC_SINGLE_ENDED 0U
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *h, uint32_t mode);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *h);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *h, uint32_t timeout);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *h);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *h);
typedef struct { uint32_t SR, CCR1, CCR2; } TIM_TypeDef;
typedef struct { uint32_t Prescaler, Period; } TIM_Base_InitTypeDef;
typedef struct { TIM_TypeDef *Instance; TIM_Base_InitTypeDef Init; uint32_t Channel; } TIM_HandleTypeDef;
extern TIM_TypeDef fake_tim2, fake_tim3;
extern TIM_HandleTypeDef htim2, htim3;
#define TIM2 (&fake_tim2)
#define TIM3 (&fake_tim3)
#define TIM_CHANNEL_1 0U
#define TIM_CHANNEL_2 4U
#define HAL_TIM_ACTIVE_CHANNEL_1 1U
#define HAL_TIM_ACTIVE_CHANNEL_2 2U
#define TIM_FLAG_CC1 2U
#define TIM_FLAG_CC1OF 512U
#define RESET 0U
#define __HAL_TIM_CLEAR_FLAG(h, f) ((h)->Instance->SR &= ~(uint32_t)(f))
#define __HAL_TIM_GET_FLAG(h, f) ((h)->Instance->SR & (f))
#define __HAL_TIM_SET_COMPARE(h, ch, v) FakeSetCompare((h), (ch), (v))
#define __HAL_TIM_GET_COMPARE(h, ch) FakeGetCompare((h), (ch))
void FakeSetCompare(TIM_HandleTypeDef *h, uint32_t channel, uint32_t value);
uint32_t FakeGetCompare(TIM_HandleTypeDef *h, uint32_t channel);
HAL_StatusTypeDef HAL_TIM_IC_Start_IT(TIM_HandleTypeDef *h, uint32_t channel);
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *h, uint32_t channel);
uint32_t HAL_TIM_ReadCapturedValue(TIM_HandleTypeDef *h, uint32_t channel);
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *h);
uint32_t HAL_GetTick(void);
uint32_t __get_PRIMASK(void);
void __disable_irq(void);
void __set_PRIMASK(uint32_t value);
#endif
