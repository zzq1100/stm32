#include "bsp_pwm.h"
#include "tim.h"

void BSP_PWM_SetDuty(uint16_t duty_centi_pct)
{
    uint32_t compare;
    if (duty_centi_pct < BSP_PWM_MIN_DUTY) duty_centi_pct = BSP_PWM_MIN_DUTY;
    if (duty_centi_pct > BSP_PWM_MAX_DUTY) duty_centi_pct = BSP_PWM_MAX_DUTY;
    compare = (BSP_PWM_PERIOD_COUNTS * (uint32_t)duty_centi_pct + 5000U) / 10000U;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, compare);
}

uint8_t BSP_PWM_Start(void)
{
    if (htim2.Init.Prescaler != 0U || htim2.Init.Period != BSP_PWM_PERIOD_COUNTS - 1U) return 0U;
    BSP_PWM_SetDuty(BSP_PWM_MIN_DUTY);
    return (uint8_t)(HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2) == HAL_OK);
}

uint16_t BSP_PWM_GetDuty(void)
{
    uint32_t compare = __HAL_TIM_GET_COMPARE(&htim2, TIM_CHANNEL_2);
    return (uint16_t)((compare * 10000U + BSP_PWM_PERIOD_COUNTS / 2U) / BSP_PWM_PERIOD_COUNTS);
}
