#ifndef BSP_PWM_H
#define BSP_PWM_H

#include <stdint.h>

#define BSP_PWM_PERIOD_COUNTS 8000U
#define BSP_PWM_MIN_DUTY 500U
#define BSP_PWM_MAX_DUTY 9500U

/* MX_TIM2_Init后启动PA1/CH2，初始5%，成功返回1。 */
uint8_t BSP_PWM_Start(void);
/* 单位0.01个百分点：500=5.00%，先限到5~95%，再就近量化CCR。
 * 不实现每秒1%的渐变；后续control决定每次传入多少。 */
void BSP_PWM_SetDuty(uint16_t duty_centi_pct);
/* 从CCR配置值反算并四舍五入到0.01%；不是对引脚的实际测量。
 * CCR预装载在下一个更新事件生效，配置与波形最多差一个周期。 */
uint16_t BSP_PWM_GetDuty(void);

#endif
