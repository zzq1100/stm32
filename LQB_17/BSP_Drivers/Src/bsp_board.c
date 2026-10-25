#include "bsp_board.h"
#include "main.h"

static ADC_HandleTypeDef adc;
static TIM_HandleTypeDef capture;
static TIM_HandleTypeDef pwm;
static volatile uint32_t period_sum, edge_tick;
static uint32_t accumulator;
static uint16_t previous;
static uint8_t samples, have_edge;

static void Check(HAL_StatusTypeDef result)
{
    if (result != HAL_OK) Error_Handler();
}

void BSP_SetLEDs(uint8_t mask)
{
    uint32_t saved = GPIOC->ODR;
    /* LCD 与 LED 共用数据总线：只在更新 LED 时打开锁存器。
       本函数与 LCD 都在主循环执行，采集中断不触碰总线。 */
    GPIOC->ODR = (saved & 0x00ffU) | ((uint32_t)(uint8_t)~mask << 8);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
    __NOP(); __NOP(); __NOP();
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
    GPIOC->ODR = saved;
}

void BSP_BoardInit(void)
{
    GPIO_InitTypeDef g = {0};
    ADC_ChannelConfTypeDef channel = {0};
    TIM_IC_InitTypeDef ic = {0};
    TIM_OC_InitTypeDef oc = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
    g.Pin = GPIO_PIN_2; g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOD, &g);
    g.Pin = 0xff00U;
    HAL_GPIO_Init(GPIOC, &g);
    BSP_SetLEDs(0);

    g.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;
    g.Mode = GPIO_MODE_INPUT; g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &g);
    g.Pin = GPIO_PIN_0; HAL_GPIO_Init(GPIOA, &g);

    /* R37 -> PB15 -> ADC2_IN15；80 MHz / 4 = 20 MHz ADC 时钟。 */
    __HAL_RCC_ADC12_CLK_ENABLE();
    g.Pin = GPIO_PIN_15; g.Mode = GPIO_MODE_ANALOG; g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &g);
    adc.Instance = ADC2;
    adc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    adc.Init.Resolution = ADC_RESOLUTION_12B;
    adc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adc.Init.ScanConvMode = ADC_SCAN_DISABLE;
    adc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    adc.Init.NbrOfConversion = 1;
    adc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    adc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    Check(HAL_ADC_Init(&adc));
    channel.Channel = ADC_CHANNEL_15;
    channel.Rank = ADC_REGULAR_RANK_1;
    channel.SamplingTime = ADC_SAMPLETIME_247CYCLES_5;
    channel.SingleDiff = ADC_SINGLE_ENDED;
    channel.OffsetNumber = ADC_OFFSET_NONE;
    Check(HAL_ADC_ConfigChannel(&adc, &channel));
    Check(HAL_ADCEx_Calibration_Start(&adc, ADC_SINGLE_ENDED));

    /* R39 -> J9 -> PB4/TIM3_CH1。1 MHz 计数、16 周期平均，
       保留 8000 Hz 以上的测量结果供 LD1 判断，不能在这里限幅。 */
    __HAL_RCC_TIM3_CLK_ENABLE();
    g.Pin = GPIO_PIN_4; g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLDOWN; g.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOB, &g);
    capture.Instance = TIM3;
    capture.Init.Prescaler = 79;
    capture.Init.CounterMode = TIM_COUNTERMODE_UP;
    capture.Init.Period = 65535;
    capture.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    Check(HAL_TIM_IC_Init(&capture));
    ic.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic.ICPrescaler = TIM_ICPSC_DIV1;
    ic.ICFilter = 4;
    Check(HAL_TIM_IC_ConfigChannel(&capture, &ic, TIM_CHANNEL_1));
    HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
    Check(HAL_TIM_IC_Start_IT(&capture, TIM_CHANNEL_1));

    /* PA1/TIM2_CH2：80 MHz / (7999+1) = 10 kHz。
       CCR 为高电平计数，PWM 初始 5%，以后只更新 CCR。 */
    __HAL_RCC_TIM2_CLK_ENABLE();
    g.Pin = GPIO_PIN_1; g.Pull = GPIO_NOPULL;
    g.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &g);
    pwm.Instance = TIM2;
    pwm.Init.Prescaler = 0;
    pwm.Init.CounterMode = TIM_COUNTERMODE_UP;
    pwm.Init.Period = 7999;
    pwm.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    pwm.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    Check(HAL_TIM_PWM_Init(&pwm));
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 400;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    Check(HAL_TIM_PWM_ConfigChannel(&pwm, &oc, TIM_CHANNEL_2));
    Check(HAL_TIM_PWM_Start(&pwm, TIM_CHANNEL_2));
}

void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&capture);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    uint16_t current, delta;
    uint32_t now;
    if (htim->Instance != TIM3 || htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1) return;
    current = (uint16_t)HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
    now = HAL_GetTick();
    if (!have_edge || (uint32_t)(now - edge_tick) >= 50U) {
        have_edge = 1; samples = 0; accumulator = 0; period_sum = 0;
    } else {
        /* 无符号减法处理 16 位计数器回绕；超过 50 ms 的周期直接失效，
           因其频率已经远低于题目规定的 800 Hz 下限。 */
        delta = (uint16_t)(current - previous);
        if (delta != 0U) {
            accumulator += delta;
            if (++samples == 16U) {
                period_sum = accumulator;
                samples = 0; accumulator = 0;
            }
        }
    }
    previous = current; edge_tick = now;
}

float BSP_ReadFrequency(uint32_t now)
{
    uint32_t sum, tick, primask = __get_PRIMASK();
    __disable_irq();
    sum = period_sum; tick = edge_tick; now = HAL_GetTick();
    __set_PRIMASK(primask);
    if (!sum || (uint32_t)(now - tick) >= 50U) return 0.0f;
    return 16000000.0f / (float)sum;
}

uint8_t BSP_ReadVoltage(float *voltage)
{
    uint32_t sum = 0, i;
    /* 4 次采样降低电位器噪声；只等待 ADC 转换，不延时等待按键。 */
    for (i = 0; i < 4; ++i) {
        if (HAL_ADC_Start(&adc) != HAL_OK) return 0;
        if (HAL_ADC_PollForConversion(&adc, 1) != HAL_OK) {
            HAL_ADC_Stop(&adc); return 0;
        }
        sum += HAL_ADC_GetValue(&adc);
        HAL_ADC_Stop(&adc);
    }
    *voltage = (float)sum * (3.3f / (4095.0f * 4.0f));
    return 1;
}

uint8_t BSP_ReadKeys(void)
{
    return (uint8_t)((~GPIOB->IDR & 7U) | ((~GPIOA->IDR & 1U) << 3));
}

void BSP_SetDuty(uint16_t duty)
{
    if (duty < 500) duty = 500;
    if (duty > 9500) duty = 9500;
    __HAL_TIM_SET_COMPARE(&pwm, TIM_CHANNEL_2, ((uint32_t)duty * 8000U + 5000U) / 10000U);
}
