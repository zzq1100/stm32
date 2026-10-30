#include "bsp_adc.h"
#include "adc.h"

static uint8_t calibrated;

uint8_t BSP_ADC_Init(void)
{
    calibrated = (HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED) == HAL_OK);
    return calibrated;
}

uint8_t BSP_ADC_Read(BSP_ADC_Sample *sample)
{
    uint32_t raw;
    if (sample == 0) return 0U;
    sample->raw = 0U;
    sample->voltage_v = 0.0f;
    sample->valid = 0U;
    if (calibrated == 0U) return 0U;
    if (HAL_ADC_Start(&hadc2) != HAL_OK) return 0U;
    if (HAL_ADC_PollForConversion(&hadc2, 1U) != HAL_OK) {
        (void)HAL_ADC_Stop(&hadc2);
        return 0U;
    }
    raw = HAL_ADC_GetValue(&hadc2);
    if (HAL_ADC_Stop(&hadc2) != HAL_OK || raw > 4095U) return 0U;

    sample->raw = (uint16_t)raw;
    /* 端点映射：0码对应0V，4095码对应题目指定的3.3V。
     * 这不是实测VDDA，也不消除板上参考电压和ADC的实际误差。 */
    sample->voltage_v = ((float)raw / 4095.0f) * 3.3f;
    sample->valid = 1U;
    return 1U;
}
